#include "controller.h"
#include "ns3/node-container.h"
#include <iostream>

using namespace ns3;

Controller::Controller() {
    m_socket = nullptr;
}

Controller::~Controller() {
    if (m_socket) {
        m_socket->Close();
    }
}

void Controller::StartApplication() {
    m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(),9));
    m_socket->SetRecvCallback(MakeCallback(&Controller::ReceiveMessageFromWorker, this));
}

void Controller::StopApplication() {
    if (m_socket) {
        m_socket->Close();
    }
}

void Controller::SendMessageToWorker(Ipv4Address address) {
    Ptr<Packet> packet = Create<Packet>(1024);
    std::cout << "Controller enviando mensagem para: " << address << std::endl;
    m_socket->SendTo(packet, 0, InetSocketAddress(address, 7));
}

void Controller::ReceiveMessageFromWorker(Ptr<Socket> socket) {
    Ptr<Packet> packet = socket->Recv();
    std::cout << "Controller recebeu um pacote de um worker!" << std::endl;
}

void Controller::SetOptions(bool balanced)
{
    m_balanced = balanced;
}

void Controller::AddWorkers(NodeContainer controlNodes)
{
    m_controlNodes = controlNodes;
    for (uint32_t i = 1; i < controlNodes.GetN(); ++i)
    {
        Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(i));
        
        db.AddNodeToDatabase(
            node->GetPower(),
            node->GetInitialConsumption(),
            node->GetCurrentConsumption(),
            node->GetCPU(),
            node->GetMemory(),
            node->GetTransmission(),
            node->GetStorage(),
            Names::FindName(node)
        );
        if (node->GetCurrentConsumption() > 0.0)
        {
            double duration = node->GetPower() / node->GetCurrentConsumption();
            finishWkrBattery[i] = ns3::Simulator::Schedule(
                ns3::Seconds(duration),
                &Controller::OutOfPower,
                this,
                i
            );
        }
    }
}

void Controller::AddApp(std::string policy, float start, float duration, double cpu, double memory, double storage)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    db.AddAppToDatabase(currentTime, policy, start, duration, cpu, memory, storage);
}

void Controller::AllocateApp(int app_id)
{
    APP application = db.SelectApplicationById(app_id);
    std::cout << "Application to be allocated:\n"
                  << "  ID: " << application.ID << "\n"
                  << "  Duration: " << application.DURATION << "\n"
                  << "  CPU: " << application.CPU << "\n"
                  << "  Memory: " << application.MEMORY << "\n"
                  << "  Storage: " << application.STORAGE << "\n"
                  << "  Policy: " << application.POLICY << "\n";
    std::vector<uint32_t> candidateNodes;
    double avgPower = 0.0;
    for (uint32_t i = 1; i < m_controlNodes.GetN(); ++i)
    {
        Ptr<CustomNode> node = DynamicCast<CustomNode>(m_controlNodes.Get(i));
        WRK worker = BuildWorkerFromNode(i);
        avgPower += worker.POWER;
        if ((worker.CPU > application.CPU) && (worker.MEMORY > application.MEMORY) && (worker.STORAGE > application.STORAGE) && (worker.POWER > 0)){
            candidateNodes.push_back(worker.ID);
        }
    }

    double currentTime = ns3::Simulator::Now().GetSeconds();
    if (candidateNodes.empty()) {
        std::cout << "Application with ID " << application.ID << " cannot be allocated " << std::endl;
        db.MarkApplicationStatus(application.ID, "3", currentTime); // Marcando com 3 para sinalizar que ainda precisa ser alocada
        return;
    }

    if (avgPower / m_controlNodes.GetN() - 1 < 50) {
        m_balanced = true;
    }

    std::sort(candidateNodes.begin(), candidateNodes.end(),
    [&](uint32_t a, uint32_t b)
    {
        WRK wa = BuildWorkerFromNode(a);
        WRK wb = BuildWorkerFromNode(b);

        if (m_balanced)
        {
            if (wa.APPS.size() != wb.APPS.size())
                return wa.APPS.size() < wb.APPS.size();
        }

        if (strcmp(application.POLICY, "performance") == 0)
            return wa.CPU > wb.CPU;

        if (strcmp(application.POLICY, "storage") == 0)
            return wa.STORAGE > wb.STORAGE;

        if (strcmp(application.POLICY, "transmission") == 0)
            return wa.TRANSMISSION > wb.TRANSMISSION;

        return false;
    });

    for (uint32_t nodeId : candidateNodes)
    {
        WRK worker = BuildWorkerFromNode(nodeId);
        std::cout << "Worker Candidate:\n"
                  << "  ID: " << worker.ID << "\n"
                  << "  Power: " << worker.POWER << "\n"
                  << "  Initial Consumption: " << worker.INITIAL_CONSUMPTION << "\n"
                  << "  Current Consumption: " << worker.CURRENT_CONSUMPTION << "\n"
                  << "  CPU: " << worker.CPU << "\n"
                  << "  Memory: " << worker.MEMORY << "\n"
                  << "  Transmission: " << worker.TRANSMISSION << "\n"
                  << "  Storage: " << worker.STORAGE << "\n";
    }

    uint32_t selectedNodeId = candidateNodes.front();
    WRK worker = BuildWorkerFromNode(selectedNodeId);
    std::cout << "Worker Selected:\n"
                  << "  ID: " << worker.ID << "\n"
                  << "  Power: " << worker.POWER << "\n"
                  << "  Initial Consumption: " << worker.INITIAL_CONSUMPTION << "\n"
                  << "  Current Consumption: " << worker.CURRENT_CONSUMPTION << "\n"
                  << "  CPU: " << worker.CPU << "\n"
                  << "  Memory: " << worker.MEMORY << "\n"
                  << "  Transmission: " << worker.TRANSMISSION << "\n"
                  << "  Storage: " << worker.STORAGE << "\n";

    db.InsertWorkerApplication(worker.ID, application.ID, currentTime);
    db.MarkApplicationStatus(application.ID, "2", currentTime); // Marcando com 2 para sinalizar que está running
    Ptr<CustomNode> node = DynamicCast<CustomNode>(m_controlNodes.Get(worker.ID));
    node->AddApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
    if (node->GetCurrentConsumption() > 0.0)
    {
        Simulator::Cancel(finishWkrBattery[worker.ID]);
        double duration = node->GetPower() / node->GetCurrentConsumption();
        finishWkrBattery[worker.ID] = ns3::Simulator::Schedule(
            ns3::Seconds(duration),
            &Controller::OutOfPower,
            this,
            worker.ID
        );
    }
    finishIDApp[application.ID] = Simulator::Schedule(
        Seconds(application.DURATION),
        &Controller::DeallocateApp,
        this,
        application.ID, 
        worker.ID,
        "1"); // Marcando com 1 para sinalizar que finalizou
    
    std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: ";
    std::cout << "allocate_worker_application called in worker " << worker.ID << " and application " << application.ID << std::endl;

}

void Controller::DeallocateApp(int idApplication, int idWorker, std::string finish)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    Ptr<CustomNode> node = DynamicCast<CustomNode>(m_controlNodes.Get(idWorker));
    APP application = db.SelectApplicationById(idApplication); 
    node->RemoveApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
    if (node->GetCurrentConsumption() > 0.0)
    {
        Simulator::Cancel(finishWkrBattery[idWorker]);
        double duration = node->GetPower() / node->GetCurrentConsumption();
        finishWkrBattery[idWorker] = ns3::Simulator::Schedule(
            ns3::Seconds(duration),
            &Controller::OutOfPower,
            this,
            idWorker
        );
    }
    db.RemoveWorkerApplication(idWorker, idApplication, currentTime);
    db.MarkApplicationStatus(idApplication, finish, currentTime);
}

void Controller::OutOfPower(int idWorker)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    std::cout << "Node with ID " << idWorker << " ran out of power at " << currentTime << "s and all applications will be removed." << std::endl;
    Ptr<CustomNode> node = DynamicCast<CustomNode>(m_controlNodes.Get(idWorker));
    std::vector<int> activeApps = node->GetApplications();
    for (int appId : activeApps)
    {
        APP application = db.SelectApplicationById(appId); 
        node->RemoveApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
        Simulator::Cancel(finishIDApp[appId]);
        db.RemoveWorkerApplication(idWorker, appId, currentTime);
        db.MarkApplicationStatus(appId, "3", currentTime); // Marcando com 3 para sinalizar que ainda precisa ser alocada
    }
}

void Controller::RechargePower(int idWorker)
{
    Ptr<CustomNode> node = DynamicCast<CustomNode>(m_controlNodes.Get(idWorker));
    node->SetPower(100.0);
    std::cout << "Node with ID " << idWorker << " was recharged at " << ns3::Simulator::Now().GetSeconds() << "s and power set to 100." << std::endl;
    std::vector<int> appIds = db.GetApplicationsToReallocate();
    for (int appId : appIds)
    {
        std::cout << "Application with ID " << appId << " will be reallocated " << std::endl;
        AllocateApp(appId);
    }
}

void Controller::BatteryMonitoringTask()
{
    for (uint32_t i = 1; i < m_controlNodes.GetN(); ++i)
    {
        WRK worker = BuildWorkerFromNode(i);
        db.UpdateNodeResources(worker);
    }
    double currentTime = Simulator::Now().GetSeconds();
    if (currentTime < 86500)
    {
        db.InsertBatteryMonitoring(currentTime);
        Simulator::Schedule(Seconds(3600), &Controller::BatteryMonitoringTask, this);
    }
}

WRK Controller::BuildWorkerFromNode(int idWorker)
{
    Ptr<CustomNode> node = DynamicCast<CustomNode>(m_controlNodes.Get(idWorker));
    node->AttPower();
    WRK worker;
    worker.ID = idWorker;
    worker.POWER   = node->GetPower();
    worker.CPU     = node->GetCPU();
    worker.MEMORY  = node->GetMemory();
    worker.STORAGE = node->GetStorage();
    worker.TRANSMISSION = node->GetTransmission();
    worker.INITIAL_CONSUMPTION = node->GetInitialConsumption();
    worker.CURRENT_CONSUMPTION = node->GetCurrentConsumption();
    worker.APPS = node->GetApplications();
    return worker;
}

void Controller::ResetDatabase()
{
    db.DropAllTables();
    db.CreateAllTables();
}
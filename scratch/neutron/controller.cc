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
                i,
                controlNodes
            );
        }
    }
}

void Controller::AddApp(std::string policy, float start, float duration, double cpu, double memory, double storage)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    db.AddAppToDatabase(currentTime, policy, start, duration, cpu, memory, storage);
}

void Controller::AllocateApp(int app_id, NodeContainer controlNodes)
{
    APP application = db.SelectApplicationById(app_id);
    std::vector<uint32_t> candidateNodes;
    for (uint32_t i = 1; i < controlNodes.GetN(); ++i)
    {
        Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(i));
        WRK worker = db.SelectWorkerById(node->GetId());
        node->AttPower();
        worker.POWER = node->GetPower();
        worker.CPU = node->GetCPU();
        worker.MEMORY = node->GetMemory();
        worker.STORAGE = node->GetStorage();
        db.UpdateNodeResources(worker);

        if ((worker.CPU > application.CPU) && (worker.MEMORY > application.MEMORY) && (worker.STORAGE > application.STORAGE) && (worker.POWER > 0) && (worker.POWER > (worker.CURRENT_CONSUMPTION+worker.INITIAL_CONSUMPTION) * application.DURATION)){
            candidateNodes.push_back(worker.ID);
        }
    }

    double currentTime = ns3::Simulator::Now().GetSeconds();
    if (candidateNodes.empty()) {
        std::cout << "Application with ID " << application.ID << " cannot be allocated " << std::endl;
        db.MarkApplicationStatus(application.ID, "3", currentTime);
        return;
    }

    uint32_t selectedNodeId = UINT32_MAX;
    double maxPower = -1.0;

    for (uint32_t nodeId : candidateNodes)
    {
        WRK worker = db.SelectWorkerById(nodeId);
        std::cout << "Worker Candidate:\n"
                  << "  ID: " << worker.ID << "\n"
                  << "  Power: " << worker.POWER << "\n"
                  << "  Initial Consumption: " << worker.INITIAL_CONSUMPTION << "\n"
                  << "  Current Consumption: " << worker.CURRENT_CONSUMPTION << "\n"
                  << "  CPU: " << worker.CPU << "\n"
                  << "  Memory: " << worker.MEMORY << "\n"
                  << "  Transmission: " << worker.TRANSMISSION << "\n"
                  << "  Storage: " << worker.STORAGE << "\n";

        if (worker.POWER > maxPower)
        {
            maxPower = worker.POWER;
            selectedNodeId = nodeId;
        }
    }

    if (selectedNodeId == UINT32_MAX) {
        std::cout << "Application with ID " << application.ID << " cannot be allocated " << std::endl;
        db.MarkApplicationStatus(application.ID, "3", currentTime);
        return;
    }

    WRK worker = db.SelectWorkerById(selectedNodeId);
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

    Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(worker.ID));
    node->AddApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
    worker.CPU = node->GetCPU();
    worker.MEMORY = node->GetMemory();
    worker.STORAGE = node->GetStorage();
    worker.CURRENT_CONSUMPTION = node->GetCurrentConsumption();
    db.UpdateNodeResources(worker);

    if (node->GetCurrentConsumption() > 0.0)
    {
        Simulator::Cancel(finishWkrBattery[worker.ID]);
        double duration = node->GetPower() / node->GetCurrentConsumption();
        finishWkrBattery[worker.ID] = ns3::Simulator::Schedule(
            ns3::Seconds(duration),
            &Controller::OutOfPower,
            this,
            worker.ID,
            controlNodes
        );
    }
    finishIDApp[application.ID] = Simulator::Schedule(
        Seconds(application.DURATION),
        &Controller::DeallocateApp,
        this,
        application.ID, 
        worker.ID,
        controlNodes,
        "1"); // Marcando com 1 para sinalizar que finalizou
    
    std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: ";
    std::cout << "allocate_worker_application called in worker " << worker.ID << " and application " << application.ID << std::endl;

}

void Controller::DeallocateApp(int idApplication, int idWorker, NodeContainer controlNodes, std::string finish)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(idWorker));
    APP application = db.SelectApplicationById(idApplication); 

    node->RemoveApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
    WRK worker = db.SelectWorkerById(node->GetId());
    worker.CPU = node->GetCPU();
    worker.MEMORY = node->GetMemory();
    worker.STORAGE = node->GetStorage();
    worker.CURRENT_CONSUMPTION = node->GetCurrentConsumption();
    db.UpdateNodeResources(worker);

    if (node->GetCurrentConsumption() > 0.0)
    {
        Simulator::Cancel(finishWkrBattery[idWorker]);
        double duration = node->GetPower() / node->GetCurrentConsumption();
        finishWkrBattery[idWorker] = ns3::Simulator::Schedule(
            ns3::Seconds(duration),
            &Controller::OutOfPower,
            this,
            idWorker,
            controlNodes
        );
    }
    db.RemoveWorkerApplication(idWorker, idApplication, currentTime);
    db.MarkApplicationStatus(idApplication, finish, currentTime);
}

void Controller::OutOfPower(int idWorker, NodeContainer controlNodes)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(idWorker));
    std::vector<int> activeApps = node->GetApplications();
    for (int appId : activeApps)
    {
        APP application = db.SelectApplicationById(appId); 
        node->RemoveApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
        WRK worker = db.SelectWorkerById(node->GetId());
        worker.CPU = node->GetCPU();
        worker.MEMORY = node->GetMemory();
        worker.STORAGE = node->GetStorage();
        worker.CURRENT_CONSUMPTION = node->GetCurrentConsumption();
        db.UpdateNodeResources(worker);
        Simulator::Cancel(finishIDApp[appId]);
        db.RemoveWorkerApplication(idWorker, appId, currentTime);
        db.MarkApplicationStatus(appId, "3", currentTime); // Marcando com 3 para sinalizar que precisará rodar novamente
    }

    std::cout << "Node with ID " << idWorker << " ran out of power at " << currentTime << "s and all applications were removed." << std::endl;
}

void Controller::RechargePower(int idWorker, NodeContainer controlNodes)
{
    Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(idWorker));
    node->SetPower(100.0);
    WRK worker = db.SelectWorkerById(node->GetId());
    worker.CURRENT_CONSUMPTION = node->GetInitialConsumption();
    worker.POWER = node->GetPower();
    db.UpdateNodeResources(worker);
    std::cout << "Node with ID " << idWorker << " was recharged at " << ns3::Simulator::Now().GetSeconds() << "s and power set to 100." << std::endl;
    std::vector<int> appIds = db.GetApplicationsToReallocate();
    for (int appId : appIds)
    {
        std::cout << "Application with ID " << appId << " will be reallocated " << std::endl;
        AllocateApp(appId, controlNodes);
    }
}

void Controller::ResetDatabase()
{
    db.DropAllTables();
    db.CreateAllTables();
}
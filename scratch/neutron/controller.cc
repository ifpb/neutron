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
    for (uint32_t i = 1; i < controlNodes.GetN(); ++i)
    {
        Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(i));
        node->AttPower();
        db.UpdateNodeResources(i, node->GetPower());
    }
    double avgPower = db.HibridPolicy();
    if (avgPower > 50.0) {
        m_balanced = false;
    } else {
        m_balanced = true;
    }
    WRK worker = db.SelectWorker(application.CPU, application.MEMORY, application.STORAGE, application.POLICY, m_balanced);
    if (worker.ID > 0 && worker.ID < static_cast<int>(controlNodes.GetN())) {

        double currentTime = ns3::Simulator::Now().GetSeconds();
        db.InsertWorkerApplication(worker.ID, application.ID, currentTime);
        db.MarkApplicationStatus(application.ID, "2", currentTime); // Marcando com 2 para sinalizar que está running
        Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(worker.ID));
        node->AddApplication(application.ID, application.CPU, application.MEMORY, application.STORAGE);
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
    else {
        double currentTime = ns3::Simulator::Now().GetSeconds();
        db.MarkApplicationStatus(application.ID, "3", currentTime);  // Marcando com 3 para sinalizar que precisará rodar novamente
    }
}

void Controller::DeallocateApp(int idApplication, int idWorker, NodeContainer controlNodes, std::string finish)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(idWorker));
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
            idWorker,
            controlNodes
        );
    }
    db.RemoveWorkerApplication(idWorker, idApplication, currentTime);
    db.MarkApplicationStatus(idApplication, finish, currentTime);

    std::vector<int> appIds = db.GetApplicationsToReallocate();
    for (int appId : appIds)
    {
        AllocateApp(appId, controlNodes);
    }
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
        Simulator::Cancel(finishIDApp[appId]);
        db.RemoveWorkerApplication(idWorker, appId, currentTime);
        db.MarkApplicationStatus(appId, "3", currentTime); // Marcando com 3 para sinalizar que precisará rodar novamente
    }

    std::vector<int> appIds = db.GetApplicationsToReallocate();
    for (int appId : appIds)
    {
        AllocateApp(appId, controlNodes);
    }
    
    //RechargePower(idWorker, controlNodes);

    std::cout << "Node with ID " << idWorker << " ran out of power at " << currentTime << "s and all applications were removed." << std::endl;
}

void Controller::RechargePower(int idWorker, NodeContainer controlNodes)
{
    Ptr<CustomNode> node = DynamicCast<CustomNode>(controlNodes.Get(idWorker));
    node->SetPower(100.0);

    //std::vector<int> appIds = db.GetApplicationsToReallocate();
    //for (int appId : appIds)
    //{
    //    AllocateApp(appId, controlNodes);
    //}

    std::cout << "Node with ID " << idWorker << " was recharged at " << ns3::Simulator::Now().GetSeconds() << "s and power set to 100." << std::endl;
}

void Controller::ResetDatabase()
{
    db.DropAllTables();
    db.CreateAllTables();
}
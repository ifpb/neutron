#include "custom-node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CustomNode");
NS_OBJECT_ENSURE_REGISTERED(CustomNode);

TypeId
CustomNode::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::CustomNode")
        .SetParent<Node>()
        .SetGroupName("Network")
        .AddConstructor<CustomNode>()
        .AddAttribute("Power",
                      "Power attribute of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_power),
                      MakeDoubleChecker<double>())
        .AddAttribute("InitialConsumption",
                      "Initial consumption of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_initialConsumption),
                      MakeDoubleChecker<double>())
        .AddAttribute("CurrentConsumption",
                      "Current consumption of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_currentConsumption),
                      MakeDoubleChecker<double>())
        .AddAttribute("CPU",
                      "CPU capacity of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_cpu),
                      MakeDoubleChecker<double>())
        .AddAttribute("Memory",
                      "Memory capacity of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_memory),
                      MakeDoubleChecker<double>())
        .AddAttribute("Transmission",
                      "Transmission capability of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_transmission),
                      MakeDoubleChecker<double>())
        .AddAttribute("Storage",
                      "Storage capacity of the node.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&CustomNode::m_storage),
                      MakeDoubleChecker<double>());
    return tid;
}

CustomNode::CustomNode()
    : m_power(0.0),
      m_initialConsumption(0.0),
      m_currentConsumption(0.0),
      m_cpu(0.0),
      m_memory(0.0),
      m_transmission(0.0),
      m_storage(0.0),
      m_lastUpdate(0.0)
{
}

CustomNode::~CustomNode() {}

double CustomNode::GetPower() const { return m_power; }
double CustomNode::GetInitialConsumption() const { return m_initialConsumption; }
double CustomNode::GetCurrentConsumption() const { return m_currentConsumption; }
double CustomNode::GetCPU() const { return m_cpu; }
double CustomNode::GetMemory() const { return m_memory; }
double CustomNode::GetTransmission() const { return m_transmission; }
double CustomNode::GetStorage() const { return m_storage; }
const std::vector<int>& CustomNode::GetApplications() const { return m_applications; }

void CustomNode::SetPower(double power) {
    m_power = power;
}

void CustomNode::AddApplication(int appId, double cpu, double mem, double storage)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    m_applications.push_back(appId);
    AttPower();
    m_currentConsumption += m_initialConsumption;
    //m_cpu -= cpu;
    //m_memory -= mem;
    //m_storage -= storage;

    std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: ";
    std::cout << "AddApplication called at CustomNode " << this->GetId() << std::endl;
    std::cout << "Aplicações alocadas até agora: [ ";
    for (int id : m_applications)
    {
        std::cout << id << " ";
    }
    std::cout << "]" << std::endl;
    std::cout << "Consumo atualizado: " << m_currentConsumption << std::endl;
}

void CustomNode::RemoveApplication(int appId, double cpu, double mem, double storage)
{
    double currentTime = ns3::Simulator::Now().GetSeconds();
    auto it = std::find(m_applications.begin(), m_applications.end(), appId);
    m_applications.erase(it);
    AttPower();
    m_currentConsumption -= m_initialConsumption;
    //m_cpu += cpu;
    //m_memory += mem;
    //m_storage += storage;

    std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: ";
    std::cout << "RemoveApplication called at node " << this->GetId() << std::endl;
    std::cout << "  > Application " << appId << " removed. Consumo atualizado: " << m_currentConsumption << std::endl;
    std::cout << "Aplicações alocadas até agora: [ ";
    for (int id : m_applications)
    {
        std::cout << id << " ";
    }
    std::cout << "]" << std::endl;
}

void CustomNode::AttPower()
{
    double now = ns3::Simulator::Now().GetSeconds();
    double timeElapsed = now - m_lastUpdate;
    double toSubtract = timeElapsed * m_currentConsumption;
    m_power -= toSubtract;
    if (m_power < 0.0)
    {
        m_power = 0.0;
    }
    m_lastUpdate = now;

    //std::cout << "At time " << now << "s: ";
    //std::cout << "Power updated for node " << this->GetId() << " | Consumption: " << m_currentConsumption << " | Power now: " << m_power << std::endl;
}

} // namespace ns3
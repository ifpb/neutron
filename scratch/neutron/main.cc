#include <fstream>
#include "ns3/core-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/application.h"
#include "ns3/random-walk-2d-mobility-model.h"
#include <yaml-cpp/yaml.h>
#include "controller.h"
#include "custom-node.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("POSITRON");

int main(int argc, char *argv[])
{

  bool logging = false;
  bool tracing = false;
  bool balanced = false;
  bool powerless = true;
  uint32_t loss = 20;
  uint32_t seed = 42;

  CommandLine cmd(__FILE__);
  cmd.AddValue("logging", "Tell control applications to logging if true", logging);
  cmd.AddValue("tracing", "Tell control applications to tracing if true", tracing);
  cmd.AddValue("balanced", "Tell control whether is a balanced policy", balanced);
  cmd.AddValue("powerless", "Set simulation scenario with battery loss", powerless);
  cmd.AddValue("loss", "Set simulation battery loss percentage", loss);
  cmd.AddValue("seed", "Set seed as an input parameter", seed);

  cmd.Parse(argc, argv);

  SeedManager::SetSeed(seed);

  if (logging)
  {
    LogComponentEnable("Database", LOG_LEVEL_INFO);
    LogComponentEnable("Controller", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }

  cout << "Carregando YAML de entrada..." << endl;
  YAML::Node input = YAML::LoadFile("./scratch/input.yaml");
  YAML::Node nodes = input["nodes"];
  YAML::Node apps = input["applications"];

  u_int32_t simulationTime = input["configs"][1]["simulationTime"].as<int>();

  NodeContainer controlNodes;
  controlNodes.Create(1);
  Names::Add("Controller", controlNodes.Get(0));

  NodeContainer workerNodes;
  int contador = 0; 
  for (auto attr : nodes)
  {
    int nodeQtd = attr["nNodes"].as<int>();
    if (nodeQtd > 0)
    {
      string nodeName = attr["name"].as<string>();
      double power = attr["power"].as<double>();
      double initialConsumption = attr["initialConsumption"].as<double>();
      double currentConsumption = attr["currentConsumption"].as<double>();
      double cpu = attr["cpu"].as<double>();
      double memory = attr["memory"].as<double>();
      double transmission = attr["transmission"].as<double>();
      double storage = attr["storage"].as<double>();

      cout << "Configurando " << nodeQtd << " nós do tipo " << nodeName << ":" << endl;
      cout << "  - Power: " << power << " W" << endl;
      cout << "  - Initial Consumption: " << initialConsumption << " W" << endl;
      cout << "  - Current Consumption: " << currentConsumption << " W" << endl;
      cout << "  - CPU: " << cpu << " GHz" << endl;
      cout << "  - Memory: " << memory << " GB" << endl;
      cout << "  - Transmission: " << transmission << " Mbps" << endl;
      cout << "  - Storage: " << storage << " GB" << endl;

      int count = 0;
      float factor = (float)loss / 100;
      int losspercent = nodeQtd * factor;

      for (int i = 0; i < nodeQtd; i++)
      {
        Ptr<CustomNode> node = CreateObject<CustomNode>();
        
        node->SetAttribute("Power", DoubleValue(power));
        node->SetAttribute("InitialConsumption", DoubleValue(initialConsumption));
        node->SetAttribute("CurrentConsumption", DoubleValue(currentConsumption));
        node->SetAttribute("CPU", DoubleValue(cpu));
        node->SetAttribute("Memory", DoubleValue(memory));
        node->SetAttribute("Transmission", DoubleValue(transmission));
        node->SetAttribute("Storage", DoubleValue(storage));
        count += 1;
        if (powerless && count <= losspercent) {
          node->SetAttribute("InitialConsumption", DoubleValue(0.001157407));
          node->SetAttribute("CurrentConsumption", DoubleValue(0.001157407));
        }

        workerNodes.Add(node);

        string auxWorkerID = attr["name"].as<string>() + to_string(i);
        Names::Add(auxWorkerID, workerNodes.Get(i+contador));

        cout << "  - Nó " << auxWorkerID << " criado no id " << i+contador << endl;
      }
      contador += nodeQtd;
    }
  }

  controlNodes.Add(workerNodes);

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  Ptr<ListPositionAllocator> nodesPositionAlloc = CreateObject<ListPositionAllocator>();
  nodesPositionAlloc->Add(Vector(0.0, 0.0, 0.0));
  nodesPositionAlloc->Add(Vector(50.0, 0.0, 0.0));
  mobility.SetPositionAllocator(nodesPositionAlloc);
  mobility.Install(controlNodes);

  LrWpanHelper ethernet;
  NetDeviceContainer controlDevices = ethernet.Install(controlNodes);
  ethernet.CreateAssociatedPan(controlDevices, 10);

  InternetStackHelper stack;
  stack.SetIpv4StackInstall(false);
  stack.Install(controlNodes);

  SixLowPanHelper sixlowpan;
  NetDeviceContainer sixlpDevices = sixlowpan.Install(controlDevices);

  Ipv6AddressHelper address;
  address.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
  Ipv6InterfaceContainer controlInterfaces = address.Assign(sixlpDevices);
  
  Ptr<Controller> controller = CreateObject<Controller>();
  controlNodes.Get(0)->AddApplication(controller);
  controller->SetStartTime(Seconds(1.0));
  controller->SetStopTime(Seconds(2*simulationTime));

  UdpEchoServerHelper workerServer(7);
  ApplicationContainer workerApps = workerServer.Install(workerNodes);
  workerApps.Start(Seconds(1.0));
  workerApps.Stop(Seconds(2*simulationTime));

  // Mandando controlNodes para que seja tratado sincronizado com os ids do banco que começam do 1 ao invés do 0

  controller->ResetDatabase();
  controller->SetOptions(balanced);
  controller->AddWorkers(controlNodes);

  for (std::size_t i = 0; i < apps.size(); i++)
  {

    std::string policy = apps[i]["policy"].as<std::string>();
    float cpu = apps[i]["cpu"].as<float>();
    float memory = apps[i]["memory"].as<float>();
    float storage = apps[i]["storage"].as<float>();
    float start = 0.0;
    float startAux = apps[i]["start"].as<float>();
    float startMin = startAux + 10;
    float startMax = startAux + 3600;
    Ptr<UniformRandomVariable> rndStart = CreateObject<UniformRandomVariable> ();
    rndStart->SetAttribute ("Min", DoubleValue (startMin));
    rndStart->SetAttribute ("Max", DoubleValue (startMax));
    start = rndStart->GetValue ();
    float duration = 0.0;
    float durationMean = apps[i]["duration"].as<float>();
    float durationVar = pow((durationMean * 0.20), 2);
    while(duration <= 0.0) {
      Ptr<NormalRandomVariable> rndDuration = CreateObject<NormalRandomVariable> ();
      rndDuration->SetAttribute ("Mean", DoubleValue (durationMean));
      rndDuration->SetAttribute ("Variance", DoubleValue (durationVar));
      duration = rndDuration->GetValue ();
    }

    int app_id = i + 1;

    controller->AddApp(policy, start, duration, cpu, memory, storage);
    
    // Mandando controlNodes para que seja tratado sincronizado com os ids do banco que começam do 1 ao invés do 0
    Simulator::Schedule(Seconds(start), &Controller::AllocateApp, controller, app_id, controlNodes);
  }

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
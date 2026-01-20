#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "database.h"
#include "custom-node.h"
#include "ns3/node-container.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/inet-socket-address.h"
#include "structs.h"

namespace ns3 {

class Controller : public Application {
public:
    Controller();
    virtual ~Controller();

    virtual void StartApplication() override;
    virtual void StopApplication() override;

    void SendMessageToWorker(Ipv4Address address);

    void SetOptions(bool balanced);
    void AddWorkers(NodeContainer workerNodes);
    void AddApp(std::string policy, float start, float duration, double cpu, double memory, double storage);
    void AllocateApp(int app_id);
    void DeallocateApp(int idApplication,int idWorker, std::string finish);
    void OutOfPower(int idWorker);
    void RechargePower(int idWorker);
    void BatteryMonitoringTask();
    WRK BuildWorkerFromNode(int idWorker);
    void ResetDatabase();

private:
    void ReceiveMessageFromWorker(Ptr<Socket> socket);
    Ptr<Socket> m_socket;
    Database db;
    ns3::EventId finishIDApp[100000];
    ns3::EventId finishWkrBattery[100000];
    bool m_balanced;
    NodeContainer m_controlNodes;
};

} // namespace ns3

#endif /* CONTROLLER_H */

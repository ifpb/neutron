#ifndef CUSTOM_NODE_H
#define CUSTOM_NODE_H

#include "ns3/application.h"
#include "ns3/node.h"
#include "ns3/core-module.h"
#include <vector>

namespace ns3 {

class CustomNode : public Node
{
public:
  static TypeId GetTypeId (void);
  CustomNode ();
  virtual ~CustomNode ();

  void SetPower (double power);
  double GetPower () const;

  void SetInitialConsumption (double initialConsumption);
  double GetInitialConsumption () const;

  void SetCurrentConsumption (double currentConsumption);
  double GetCurrentConsumption () const;

  void SetCPU (double cpu);
  double GetCPU () const;

  void SetMemory (double memory);
  double GetMemory () const;

  void SetTransmission (double transmission);
  double GetTransmission () const;

  void SetStorage (double storage);
  double GetStorage () const;

  void AddApplication(int appId, double cpu, double mem, double storage);
  void RemoveApplication(int appId, double cpu, double mem, double storage);
  const std::vector<int>& GetApplications() const;

  void AttPower ();

private:
  double m_power;
  double m_initialConsumption;
  double m_currentConsumption;
  double m_cpu;
  double m_memory;
  double m_transmission;
  double m_storage;
  double m_lastUpdate;
  std::vector<int> m_applications;

};

} // namespace ns3

#endif // CUSTOM_NODE_H
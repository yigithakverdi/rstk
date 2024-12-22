#pragma once

#include "engine/topology/topology.hpp"
#include "plugins/plugins.hpp"
#include "router/route.hpp"
#include <functional>
#include <string>
#include <vector>

class BasePolicyEngine : public PolicyEngine {
public:
  bool shouldAcceptRoute(const Route &route) const override;
  bool shouldPreferRoute(const Route &currentRoute,
                         const Route &newRoute) const override;
  bool canForwardRoute(Relation sourceRelation,
                       Relation targetRelation) const override;

protected:
  virtual int calculateLocalPreference(const Route &route) const;
  virtual int calculateASPathLength(const Route &route) const;
  virtual int getNextHopASNumber(const Route &route) const;

private:
  std::vector<std::function<int(const Route &)>> getPreferenceRules() const;
};

class BaseProtocol : public Protocol {
public:
  BaseProtocol();

  std::string getProtocolName() const override;
  std::string getProtocolInfo() const override { return "Base BGP Protocol\n"; }
  std::string getDetailedProtocolInfo() const override;
  std::pair<int, int>
  getDeploymentStats(const Topology &topology) const override;
};

class BaseDeploymentStrategy : public DeploymentStrategy {
public:
  BaseDeploymentStrategy() = default;
  void deploy(Topology &topology) override;
  void clear(Topology &topology) override;
  bool validate(const Topology &topology) const override;
};

#pragma once

#include "engine/topology/topology.hpp"
#include "plugins/plugins.hpp"
#include "router/route.hpp"
#include <functional>
#include <string>
#include <vector>

class LeakPolicyEngine : public PolicyEngine {
public:
  bool shouldAcceptRoute(const Route &route) const override;
  bool shouldPreferRoute(const Route &currentRoute, const Route &newRoute) const override;
  bool canForwardRoute(Relation sourceRelation, Relation targetRelation) const override;

private:
  std::vector<std::function<int(const Route &)>> getPreferenceRules() const;
  int calculateLocalPreference(const Route &route) const;
  int calculateASPathLength(const Route &route) const;
  int getNextHopASNumber(const Route &route) const;
};

class LeakProtocol : public Protocol {
public:
  LeakProtocol();

  std::string getProtocolName() const override;
  std::string getProtocolInfo() const override { return "Route Leak Protocol\n"; }
  std::string getDetailedProtocolInfo() const override;
  std::pair<int, int> getDeploymentStats(const Topology &topology) const override;
};

class LeakDeploymentStrategy : public DeploymentStrategy {
public:
  LeakDeploymentStrategy() = default;
  void deploy(Topology &topology) override;
  void clear(Topology &topology) override;
  bool validate(const Topology &topology) const override;
};

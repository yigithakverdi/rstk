#pragma once

#include "engine/topology/deployment.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/aspa/aspa.hpp"
#include "plugins/base/base.hpp"
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

class RandomLeakDeployment : public DeploymentStrategy {
public:
  RandomLeakDeployment(double objectPercentage, double policyPercentage)
      : objectPercentage_(objectPercentage), policyPercentage_(policyPercentage) {}

  void deploy(Topology &topology) override {
    clear(topology);

    size_t totalRouters = topology.G->nodes.size();
    size_t objectCount = static_cast<size_t>(totalRouters * objectPercentage_ / 100.0);
    size_t policyCount = static_cast<size_t>(totalRouters * policyPercentage_ / 100.0);

    auto objectRouters = topology.RandomSampleRouters(objectCount);
    auto policyRouters = topology.RandomSampleRouters(policyCount);

    // Deploy ASPA with leak potential
    for (const auto &router : policyRouters) {
      router->proto = std::make_unique<ASPAProtocol>(topology.RPKIInstance);
      if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->proto.get())) {
        ASPAObject obj = ASPAProtocol::createObjectForRouter(router);
        aspaProto->addASPAObject(obj);
        aspaProto->updateUSPAS();
      }
    }

    // Create ASPA objects for non-ASPA routers
    for (const auto &router : objectRouters) {
      if (!dynamic_cast<ASPAProtocol *>(router->proto.get())) {
        ASPAProtocol::createObjectInRPKI(router, topology.RPKIInstance);
      }
    }
  }

  void clear(Topology &topology) override {
    for (const auto &[id, router] : topology.G->nodes) {
      if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->proto.get())) {
        aspaProto->clearASPAObjects();
      }
    }
    topology.RPKIInstance->USPAS.clear();
  }

  bool validate(const Topology &topology) const override {
    return true; // Add actual validation if needed
  }

private:
  double objectPercentage_;
  double policyPercentage_;
};

class SelectiveLeakDeployment : public DeploymentStrategy {
public:
  SelectiveLeakDeployment(double objectPercentage, double policyPercentage)
      : objectPercentage_(objectPercentage), policyPercentage_(policyPercentage) {}

  void deploy(Topology &topology) override {
    clear(topology);

    auto routersByCustomerDegree = topology.GetByCustomerDegree();
    size_t totalRouters = topology.G->nodes.size();
    size_t totalObjectCount = static_cast<size_t>(totalRouters * objectPercentage_ / 100.0);
    size_t totalPolicyCount = static_cast<size_t>(totalRouters * policyPercentage_ / 100.0);

    std::vector<std::shared_ptr<Router>> objectTargets(
        routersByCustomerDegree.begin(),
        routersByCustomerDegree.begin() +
            std::min(totalObjectCount, routersByCustomerDegree.size()));

    std::vector<std::shared_ptr<Router>> policyTargets(
        routersByCustomerDegree.begin(),
        routersByCustomerDegree.begin() +
            std::min(totalPolicyCount, routersByCustomerDegree.size()));

    // Deploy ASPA protocols and objects
    for (const auto &router : policyTargets) {
      router->proto = std::make_unique<ASPAProtocol>(topology.RPKIInstance);
      if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->proto.get())) {
        ASPAObject obj = ASPAProtocol::createObjectForRouter(router);
        aspaProto->addASPAObject(obj);
        aspaProto->updateUSPAS();
      }
    }

    // Create ASPA objects for non-ASPA routers
    for (const auto &router : objectTargets) {
      if (!dynamic_cast<ASPAProtocol *>(router->proto.get())) {
        ASPAProtocol::createObjectInRPKI(router, topology.RPKIInstance);
      }
    }
  }

  void clear(Topology &topology) override {
    for (const auto &[id, router] : topology.G->nodes) {
      if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->proto.get())) {
        aspaProto->clearASPAObjects();
      }
    }
    topology.RPKIInstance->USPAS.clear();
  }

  bool validate(const Topology &topology) const override {
    return true; // Add actual validation if needed
  }

private:
  double objectPercentage_;
  double policyPercentage_;
};

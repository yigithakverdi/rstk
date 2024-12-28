// aspa.hpp
#pragma once

#ifndef ASPA_HPP
#define ASPA_HPP

#include "engine/rpki/rpki.hpp"
#include "engine/topology/deployment.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/plugins.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum class ASPAResult { Valid, Invalid, Unknown };
enum class ASPAAuthResult { ProviderPlus, NotProviderPlus, NoAttestation, Peer };

class ASPAProtocol;

class ASPAPolicyEngine : public PolicyEngine {
public:
  explicit ASPAPolicyEngine(std::shared_ptr<RPKI> rpki);
  ASPAPolicyEngine(std::shared_ptr<RPKI> rpki, std::shared_ptr<ASPAProtocol> aspaProtocol);

  bool shouldAcceptRoute(const Route &route) const override;
  bool shouldPreferRoute(const Route &currentRoute, const Route &newRoute) const override;
  ASPAResult PerformASPA(const Route &route) const;
  bool HasASPARecord(Router *as) const;
  bool IsProviderPlus(Router *as, Router *provider) const;

private:
  std::shared_ptr<RPKI> rpki_;
  std::vector<ASPAObject> aspaObjects_;

  std::vector<std::function<int(const Route &)>> PreferenceRules() const;
  int localPref(const Route &route) const;
  int asPathLength(const Route &route) const;
  int nextHopASNumber(const Route &route) const;
  bool canForwardRoute(Relation sourceRelation, Relation targetRelation) const override;
};

class ASPAProtocol : public Protocol, public std::enable_shared_from_this<ASPAProtocol> {
public:
  ASPAProtocol(std::shared_ptr<RPKI> rpki);
  void addASPAObject(const ASPAObject &obj);
  std::vector<ASPAObject> getAllASPAObjects() const;
  ASPAObject getASPAObject(Router *customerAS) const;
  std::string getProtocolName() const override;
  void updateUSPAS();
  std::string getProtocolInfo() const override;
  std::string getDetailedProtocolInfo() const override;
  std::pair<int, int> getDeploymentStats(const Topology &topology) const override;

  static bool hasASPAObject(const Topology &topology, int asNumber) {
    return topology.RPKIInstance->USPAS.find(asNumber) != topology.RPKIInstance->USPAS.end();
  }

  static bool hasASPAPolicy(const Router *router) {
    return router->proto && router->proto->getProtocolName() == "ASPA";
  }

  static ASPAObject createObjectForRouter(std::shared_ptr<Router> router) {
    std::vector<int> providerASes;
    if (router->Tier == 1) {
      providerASes = {0};
    } else {
      for (const auto &provider : router->GetProviders()) {
        providerASes.push_back(provider.router->ASNumber);
      }
    }
    return ASPAObject(router->ASNumber, providerASes, std::vector<unsigned char>());
  }

  static void createObjectInRPKI(std::shared_ptr<Router> router, std::shared_ptr<RPKI> rpki) {
    if (dynamic_cast<ASPAProtocol *>(router->proto.get()) != nullptr) {
      std::cerr << "Warning: Creating ASPA object for ASPA-enabled router" << std::endl;
    }
    rpki->USPAS[router->ASNumber] = createObjectForRouter(router);
  }
  void clearASPAObjects();

private:
  std::shared_ptr<RPKI> rpki_;
  std::vector<ASPAObject> aspaSet_;
};

class RandomDeployment : public DeploymentStrategy {
public:
  RandomDeployment(double objectPercentage, double policyPercentage)
      : objectPercentage_(objectPercentage), policyPercentage_(policyPercentage) {}

  void deploy(Topology &topology) override;
  void clear(Topology &topology) override;
  bool validate(const Topology &topology) const override;

private:
  double objectPercentage_;
  double policyPercentage_;
};

class SelectiveDeployment : public DeploymentStrategy {
public:
  SelectiveDeployment(double objectPercentage, double policyPercentage)
      : objectPercentage_(objectPercentage), policyPercentage_(policyPercentage) {}

  void deploy(Topology &topology) override;
  void clear(Topology &topology) override;
  bool validate(const Topology &topology) const override;

private:
  double objectPercentage_;
  double policyPercentage_;

  void addTierToTargets(const std::vector<std::shared_ptr<Router>> &tier, size_t remainingCount,
                        std::vector<std::shared_ptr<Router>> &targets) {
    size_t toAdd = std::min(tier.size(), remainingCount);
    targets.insert(targets.end(), tier.begin(), tier.begin() + toAdd);
  }
};

#endif // ASPA_HPP

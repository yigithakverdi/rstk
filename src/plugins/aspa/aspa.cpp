#include "plugins/aspa/aspa.hpp"
#include "engine/rpki/rpki.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include "logger/logger.hpp"

ASPAObject::ASPAObject(int customerAS, const std::vector<int> &providerASes,
                       const std::vector<unsigned char> &signature)
    : customerAS(customerAS), providerASes(providerASes), signature(signature) {}

ASPAPolicyEngine::ASPAPolicyEngine(std::shared_ptr<RPKI> rpki) : rpki_(rpki) {}

ASPAPolicyEngine::ASPAPolicyEngine(std::shared_ptr<RPKI> rpki,
                                   std::shared_ptr<ASPAProtocol> aspaProtocol)
    : rpki_(rpki) {}

bool ASPAPolicyEngine::shouldAcceptRoute(const Route &route) const {
  ASPAResult result = PerformASPA(route);

  if (route.ContainsCycle() && result == ASPAResult::Invalid) {
    return false;
  }
  return result != ASPAResult::Invalid;
}

bool ASPAPolicyEngine::shouldPreferRoute(const Route &currentRoute, const Route &newRoute) const {
  if (currentRoute.destination != newRoute.destination) {
    return false;
  }

  for (const auto &rule : PreferenceRules()) {
    int currentVal = rule(currentRoute);
    int newVal = rule(newRoute);

    if (currentVal != -1 && newVal != -1) {
      if (currentVal < newVal) {
        return false;
      }
      if (newVal < currentVal) {
        return true;
      }
    }
  }
  return false;
}

void ASPAProtocol::updateUSPAS() {
  std::map<int, std::vector<std::vector<int>>> customerProviders;

  for (const auto &obj : aspaSet_) {
    customerProviders[obj.customerAS].push_back(obj.providerASes);
  }

  for (const auto &[customerAS, providerSets] : customerProviders) {
    std::vector<int> unionProviders;
    for (const auto &providers : providerSets) {
      for (int provider : providers) {
        if (std::find(unionProviders.begin(), unionProviders.end(), provider) ==
            unionProviders.end()) {
          unionProviders.push_back(provider);
        }
      }
    }

    ASPAObject unionObj(customerAS, unionProviders, std::vector<unsigned char>());
    rpki_->USPAS[customerAS] = unionObj;
  }
}

std::vector<std::function<int(const Route &)>> ASPAPolicyEngine::PreferenceRules() const {
  return {[this](const Route &route) { return localPref(route); },
          [this](const Route &route) { return asPathLength(route); },
          [this](const Route &route) { return nextHopASNumber(route); }};
}

int ASPAPolicyEngine::localPref(const Route &route) const {
  if (route.path.size() < 2) {
    return -1;
  }

  Router *finalRouter = route.destination;
  Router *firstHopRouter = route.path[route.path.size() - 2];

  Relation relation = finalRouter->GetRelation(firstHopRouter);

  if (relation != Relation::Unknown) {
    return static_cast<int>(relation);
  }

  return -1;
}

int ASPAPolicyEngine::asPathLength(const Route &route) const {
  return static_cast<int>(route.path.size());
}

int ASPAPolicyEngine::nextHopASNumber(const Route &route) const {
  if (route.path.size() < 2) {
    return -1;
  }

  Router *nextHopRouter = route.path[route.path.size() - 2];
  return nextHopRouter->ASNumber;
}

bool ASPAPolicyEngine::canForwardRoute(Relation first, Relation relation) const {
  if (first == Relation::Customer) {
    return false;
  }
  return (first == Relation::Customer) || (relation == Relation::Customer);
}

ASPAProtocol::ASPAProtocol(std::shared_ptr<RPKI> rpki)
    : Protocol(std::make_unique<ASPAPolicyEngine>(rpki)), rpki_(rpki) {}

void ASPAProtocol::addASPAObject(const ASPAObject &obj) {
  aspaSet_.push_back(obj);
  updateUSPAS();
}

std::vector<ASPAObject> ASPAProtocol::getAllASPAObjects() const { return aspaSet_; }

ASPAObject ASPAProtocol::getASPAObject(Router *customerAS) const {
  for (const auto &obj : aspaSet_) {
    if (obj.customerAS == customerAS->ASNumber) {
      return obj;
    }
  }
  return ASPAObject();
}

std::string ASPAProtocol::getProtocolName() const { return "ASPA"; }

ASPAResult ASPAPolicyEngine::PerformASPA(const Route &route) const {
    if (route.path.size() < 2) {
        throw std::runtime_error("Route length below verifiable!");
    }

    Router *final = route.path.back();
    Router *firstHop = route.path[route.path.size() - 2];
    Relation relation = final->GetRelation(firstHop);

    // Customer/Peer path validation
    if (relation == Relation::Customer || relation == Relation::Peer) {
        if (route.path.size() == 2) return ASPAResult::Valid;

        // Check all ASPA records exist first
        for (size_t i = 0; i < route.path.size() - 2; i++) {
            if (!HasASPARecord(route.path[i])) {
                return ASPAResult::Unknown;
            }
        }

        // Validate provider relationships
        for (size_t i = 0; i < route.path.size() - 2; i++) {
            if (!IsProviderPlus(route.path[i], route.path[i + 1])) {
                return ASPAResult::Invalid;
            }
        }
        return ASPAResult::Valid;
    }

    // Provider path validation
    if (relation == Relation::Provider) {
        if (route.path.size() <= 3) return ASPAResult::Valid;

        // Check all ASPA records exist first
        for (size_t i = 0; i < route.path.size() - 2; i++) {
            if (!HasASPARecord(route.path[i])) {
                return ASPAResult::Unknown;
            }
        }

        // Find invalid hops
        size_t u_min = route.path.size();
        size_t v_max = 0;

        for (size_t i = 0; i < route.path.size() - 2; i++) {
            if (!IsProviderPlus(route.path[i], route.path[i + 1])) {
                u_min = i + 2;
                break;
            }
        }

        for (size_t i = route.path.size() - 2; i > 0; i--) {
            if (!IsProviderPlus(route.path[i], route.path[i - 1])) {
                v_max = i;
                break;
            }
        }

        if (u_min <= v_max) {
            return ASPAResult::Invalid;
        }

        return ASPAResult::Valid;
    }

    return ASPAResult::Invalid;
}

// Helper methods
bool ASPAPolicyEngine::HasASPARecord(Router* as) const {
    return rpki_->USPAS.find(as->ASNumber) != rpki_->USPAS.end();
}

bool ASPAPolicyEngine::IsProviderPlus(Router* as, Router* provider) const {
    auto uspasIt = rpki_->USPAS.find(as->ASNumber);
    if (uspasIt == rpki_->USPAS.end()) {
        return false;
    }

    const auto& providers = uspasIt->second.providerASes;
    return std::find(providers.begin(), providers.end(), provider->ASNumber) != providers.end();
}

std::string ASPAProtocol::getProtocolInfo() const {
  std::stringstream ss;
  ss << "ASPA Protocol\n";
  return ss.str();
}

std::string ASPAProtocol::getDetailedProtocolInfo() const {
  std::stringstream ss;
  ss << getProtocolInfo();
  ss << "ASPA Objects: " << aspaSet_.size() << "\n";
  // Add more detailed ASPA-specific info
  return ss.str();
}

std::pair<int, int> ASPAProtocol::getDeploymentStats(const Topology &topology) const {
  int objectCount = 0;
  int policyCount = 0;

  for (const auto &[id, router] : topology.G->nodes) {
    if (router->proto && router->proto->getProtocolName() == "ASPA") {
      policyCount++;
    }
    if (topology.RPKIInstance->USPAS.find(id) != topology.RPKIInstance->USPAS.end()) {
      objectCount++;
    }
  }

  return {objectCount, policyCount};
}

void ASPAProtocol::clearASPAObjects() { aspaSet_.clear(); }

void ASPADeployment::deploy(Topology &topology) {
  // Clear any existing deployment first
  clear(topology);

  // Calculate number of routers for each deployment
  size_t totalRouters = topology.G->nodes.size();
  size_t objectCount = static_cast<size_t>(totalRouters * objectPercentage_ / 100.0);
  size_t policyCount = static_cast<size_t>(totalRouters * policyPercentage_ / 100.0);

  auto objectRouters = topology.RandomSampleRouters(objectCount);
  auto policyRouters = topology.RandomSampleRouters(policyCount);

  // Deploy ASPA protocols and objects
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

void ASPADeployment::clear(Topology &topology) {
  for (const auto &[id, router] : topology.G->nodes) {
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->proto.get())) {
      aspaProto->clearASPAObjects();
    }
  }

  // Clear ASPA objects from RPKI
  topology.RPKIInstance->USPAS.clear();
}

bool ASPADeployment::validate(const Topology &topology) const {
  // Validation logic
  return true;
}

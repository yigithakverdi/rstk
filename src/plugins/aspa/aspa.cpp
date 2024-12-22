#include "plugins/aspa/aspa.hpp"
#include "engine/rpki/rpki.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include "router/router.hpp"

ASPAObject::ASPAObject(int customerAS, const std::vector<int> &providerASes,
                       const std::vector<unsigned char> &signature)
    : customerAS(customerAS), providerASes(providerASes), signature(signature) {
}

ASPAPolicyEngine::ASPAPolicyEngine(std::shared_ptr<RPKI> rpki) : rpki_(rpki) {}

ASPAPolicyEngine::ASPAPolicyEngine(std::shared_ptr<RPKI> rpki,
                                   std::shared_ptr<ASPAProtocol> aspaProtocol)
    : rpki_(rpki) {}

bool ASPAPolicyEngine::shouldAcceptRoute(const Route &route) const {
  ASPAResult result = performASPAVerification(route);

  if (route.ContainsCycle() && result == ASPAResult::Invalid) {
    return false;
  }
  return result != ASPAResult::Invalid;
}

bool ASPAPolicyEngine::shouldPreferRoute(const Route &currentRoute,
                                         const Route &newRoute) const {
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

    ASPAObject unionObj(customerAS, unionProviders,
                        std::vector<unsigned char>());
    rpki_->USPAS[customerAS] = unionObj;
  }
}

std::vector<std::function<int(const Route &)>>
ASPAPolicyEngine::PreferenceRules() const {
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

bool ASPAPolicyEngine::canForwardRoute(Relation first,
                                       Relation relation) const {
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

std::vector<ASPAObject> ASPAProtocol::getAllASPAObjects() const {
  return aspaSet_;
}

ASPAObject ASPAProtocol::getASPAObject(Router *customerAS) const {
  for (const auto &obj : aspaSet_) {
    if (obj.customerAS == customerAS->ASNumber) {
      return obj;
    }
  }
  return ASPAObject();
}

std::string ASPAProtocol::getProtocolName() const { return "ASPA"; }

ASPAResult ASPAPolicyEngine::performASPAVerification(const Route &route) const {
  Route mutableRoute = route;
  mutableRoute.ReversePath();

  if (mutableRoute.path.size() < 2) {
    return ASPAResult::Invalid;
  }

  Router *verifying = mutableRoute.path[0];
  Router *neighbor = mutableRoute.path[1];

  Relation relation = verifying->GetRelation(neighbor);
  size_t N = mutableRoute.path.size();
  if (N < 2) {
    return ASPAResult::Invalid;
  }

  ASPAResult result;
  switch (relation) {
  case Relation::Customer:
  case Relation::Peer:
    result = this->upstreamPathVerification(mutableRoute);
    break;

  case Relation::Provider:
    result = this->downstreamPathVerification(mutableRoute);
    break;
  default:
    return ASPAResult::Invalid;
  }
  return result;
}

std::pair<int, int> ASPAPolicyEngine::computeUpRamp(const Route &route) const {
  int N = route.path.size();
  int maxUpRamp = N;
  int minUpRamp = N;

  for (int i = 0; i + 1 < N; ++i) {
    Router *customerAS = route.path[i + 1];
    Router *providerAS = route.path[i];
    ASPAAuthResult authResult = authorized(customerAS, providerAS);

    if (authResult == ASPAAuthResult::Peer) {
      if (i == 0) {
        maxUpRamp = N;
        minUpRamp = N;
      } else {
        maxUpRamp = i + 1;
        minUpRamp = i + 1;
      }
      break;
    }

    if (authResult == ASPAAuthResult::NotProviderPlus && maxUpRamp == N) {
      maxUpRamp = i + 1;
    }

    if ((authResult == ASPAAuthResult::NotProviderPlus ||
         authResult == ASPAAuthResult::NoAttestation) &&
        minUpRamp == N) {
      minUpRamp = i + 1;
    }
  }

  return std::make_pair(maxUpRamp, minUpRamp);
}

std::pair<int, int>
ASPAPolicyEngine::computeDownRamp(const Route &route) const {
  int N = route.path.size();
  int maxDownRamp = N;
  int minDownRamp = N;

  for (int i = N - 1; i > 0; --i) {
    Router *customerAS = route.path[i];
    Router *providerAS = route.path[i - 1];

    ASPAAuthResult authResult = authorized(customerAS, providerAS);

    if (authResult == ASPAAuthResult::Peer) {
      maxDownRamp = N - i;
      minDownRamp = N - i;
      break;
    }

    if (authResult == ASPAAuthResult::NotProviderPlus && maxDownRamp == N) {
      maxDownRamp = N - i;
    }

    if ((authResult == ASPAAuthResult::NotProviderPlus ||
         authResult == ASPAAuthResult::NoAttestation) &&
        minDownRamp == N) {
      minDownRamp = N - i;
    }
  }
  return std::make_pair(maxDownRamp, minDownRamp);
}

ASPAAuthResult ASPAPolicyEngine::authorized(Router *currAS,
                                            Router *nextAS) const {
  std::stringstream log_msg;
  Relation relation = currAS->GetRelation(nextAS);

  if (relation == Relation::Peer) {
    return ASPAAuthResult::Peer;
  }

  auto uspasIt = rpki_->USPAS.find(currAS->ASNumber);
  if (uspasIt == rpki_->USPAS.end()) {
    return ASPAAuthResult::NoAttestation;
  }

  const auto &providers = uspasIt->second.providerASes;
  for (int provider : providers) {
    log_msg << provider << " ";
  }

  bool isProviderAuthorized = std::find(providers.begin(), providers.end(),
                                        nextAS->ASNumber) != providers.end();

  if (isProviderAuthorized) {
    return ASPAAuthResult::ProviderPlus;
  }

  return ASPAAuthResult::NotProviderPlus;
}

ASPAResult
ASPAPolicyEngine::upstreamPathVerification(const Route &route) const {
  int N = route.path.size();

  auto [maxUpRamp, minUpRamp] = computeUpRamp(route);

  int maxDownRamp = 0;
  int minDownRamp = 0;

  if (maxUpRamp < N) {
    return ASPAResult::Invalid;
  } else if (minUpRamp < N) {
    return ASPAResult::Unknown;
  } else {
    return ASPAResult::Valid;
  }
}

ASPAResult
ASPAPolicyEngine::downstreamPathVerification(const Route &route) const {
  int N = route.path.size();

  auto [maxUpRamp, minUpRamp] = computeUpRamp(route);
  auto [maxDownRamp, minDownRamp] = computeDownRamp(route);

  if (maxUpRamp + maxDownRamp < N) {
    return ASPAResult::Invalid;
  } else if (minUpRamp + minDownRamp < N) {
    return ASPAResult::Unknown;
  } else {
    return ASPAResult::Valid;
  }
}

ASPAResult ASPAPolicyEngine::PerformASPA(const Route &route) const {
  Router *final = route.path.back();
  Router *firstHop = route.path[route.path.size() - 2];

  if (final == nullptr || route.path.empty()) {
    throw std::runtime_error("Route is not properly defined or empty");
  }

  Relation relation = final->GetRelation(firstHop);
  ASPAResult result = ASPAResult::Unknown;

  auto checkAuth = [this](Router *currAS, Router *nextAS) -> ASPAAuthResult {
    return this->authorized(currAS, nextAS);
  };

  if (relation == Relation::Customer || relation == Relation::Peer) {
    if (route.path.size() < 2) {
      throw std::runtime_error("Route length below verifyable!");
    } else if (route.path.size() == 2) {
      result = ASPAResult::Valid;
    } else {
      result = ASPAResult::Valid;
      for (size_t i = 0; i < route.path.size(); i++) {
        if (i + 1 < route.path.size()) {
          Router *curr_asys = route.path[i];
          Router *next_asys = route.path[i + 1];

          ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
          if (auth == ASPAAuthResult::NotProviderPlus) {
            result = ASPAResult::Invalid;
            break;
          } else if (auth == ASPAAuthResult::NoAttestation) {
            result = ASPAResult::Unknown;
            break;
          } else {
            continue;
          }
        }
      }
    }

  } else if (relation == Relation::Provider) {
    if (route.path.size() < 2) {
      throw std::runtime_error("Route length below verifyable!");
    } else if (route.path.size() == 2 || route.path.size() == 3) {
      result = ASPAResult::Valid;
    } else {
      size_t u_min = route.path.size();
      for (size_t i = 0; i < route.path.size(); i++) {
        if (i + 1 < route.path.size()) {
          Router *curr_asys = route.path[i];
          Router *next_asys = route.path[i + 1];

          ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
          if (auth == ASPAAuthResult::NotProviderPlus) {
            u_min = i + 2;
            break;
          }
        }
      }

      size_t v_max = 0;
      for (size_t i = 0; i < route.path.size(); i++) {
        if (i == 0)
          continue;
        if (i + 1 < route.path.size()) {
          size_t curr_idx = route.path.size() - 1 - i;
          size_t next_idx = route.path.size() - 1 - (i + 1);

          Router *curr_asys = route.path[curr_idx];
          Router *next_asys = route.path[next_idx];

          ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
          if (auth == ASPAAuthResult::NotProviderPlus) {
            v_max = next_idx + 1;
            break;
          }
        }
      }

      if (u_min <= v_max) {
        result = ASPAResult::Invalid;
      }

      if (result != ASPAResult::Invalid) {
        size_t k = 1;
        for (size_t i = 0; i < route.path.size(); i++) {
          if (i + 1 < route.path.size()) {
            Router *curr_asys = route.path[i];
            Router *next_asys = route.path[i + 1];
            ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
            if (auth == ASPAAuthResult::ProviderPlus) {
              k = i + 2;
            } else {
              break;
            }
          }
        }

        size_t l = route.path.size() - 1;
        for (size_t i = 0; i < route.path.size(); i++) {
          if (i == 0)
            continue;
          if (i + 1 < route.path.size()) {
            size_t curr_idx = route.path.size() - 1 - i;
            size_t next_idx = route.path.size() - 1 - (i + 1);

            Router *curr_asys = route.path[curr_idx];
            Router *next_asys = route.path[next_idx];
            ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
            if (auth == ASPAAuthResult::ProviderPlus) {
              l = next_idx + 1;
            } else {
              break;
            }
          }
        }

        if (static_cast<int>(l) - static_cast<int>(k) <= 1) {
          result = ASPAResult::Valid;
        } else {
          result = ASPAResult::Unknown;
        }
      }
    }

  } else {
    throw std::runtime_error("Unknown relationship type");
  }

  return result;
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

std::pair<int, int>
ASPAProtocol::getDeploymentStats(const Topology &topology) const {
  int objectCount = 0;
  int policyCount = 0;

  for (const auto &[id, router] : topology.G->nodes) {
    if (router->proto && router->proto->getProtocolName() == "ASPA") {
      policyCount++;
    }
    if (topology.RPKIInstance->USPAS.find(id) !=
        topology.RPKIInstance->USPAS.end()) {
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
  size_t objectCount =
      static_cast<size_t>(totalRouters * objectPercentage_ / 100.0);
  size_t policyCount =
      static_cast<size_t>(totalRouters * policyPercentage_ / 100.0);

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

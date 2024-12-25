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
  ASPAResult result = performASPAVerification(route);

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

std::pair<int, int> ASPAPolicyEngine::computeDownRamp(const Route &route) const {
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

ASPAAuthResult ASPAPolicyEngine::authorized(Router *currAS, Router *nextAS) const {
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

  bool isProviderAuthorized =
      std::find(providers.begin(), providers.end(), nextAS->ASNumber) != providers.end();

  if (isProviderAuthorized) {
    return ASPAAuthResult::ProviderPlus;
  }

  return ASPAAuthResult::NotProviderPlus;
}

ASPAResult ASPAPolicyEngine::upstreamPathVerification(const Route &route) const {
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

ASPAResult ASPAPolicyEngine::downstreamPathVerification(const Route &route) const {
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
  LOG.debug("Performing ASPA verification for route: " + route.ToString());

  ASPAResult result = ASPAResult::Valid;
  bool hasNoAttestation = false;
  LOG.debug("Final: " + std::to_string(final->ASNumber) + " FirstHop: " + std::to_string(firstHop->ASNumber));

  if (final == nullptr || route.path.empty()) {
    LOG.error("Route is not properly defined or empty");
    throw std::runtime_error("Route is not properly defined or empty");
  }

  Relation relation = final->GetRelation(firstHop);
  LOG.debug("Relation: " + std::to_string(static_cast<int>(relation)));
  if (relation == Relation::Unknown) {
    LOG.error("Unknown relation between final and first hop");
    return ASPAResult::Unknown;
  }

  if (route.path.size() < 2) {
    LOG.error("Route length below verifiable!");
    throw std::runtime_error("Route length below verifiable!");
  }

  if (route.path.size() == 2) {
    LOG.debug("Route length is 2, no need for ASPA verification");
    return ASPAResult::Valid;
  }

  auto checkAuth = [this](Router *currAS, Router *nextAS) -> ASPAAuthResult {
    LOG.debug("Checking authorization between " + std::to_string(currAS->ASNumber) + " and " + std::to_string(nextAS->ASNumber));
    return this->authorized(currAS, nextAS);
  };

  if (relation == Relation::Customer || relation == Relation::Peer) {
    LOG.debug("Performing upstream path verification");
    bool seenPeer = false;

    for (size_t i = 0; i < route.path.size() - 1; i++) {
      Router *curr_asys = route.path[i];
      Router *next_asys = route.path[i + 1];
      LOG.debug("Checking authorization between " + std::to_string(curr_asys->ASNumber) + " and " 
                + std::to_string(next_asys->ASNumber));

      ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
      LOG.debug("Authorization result: " + std::to_string(static_cast<int>(auth)));

      if (auth == ASPAAuthResult::Peer) {
        LOG.debug("Peer relation found");
        if (seenPeer) {
          LOG.debug("It's the second peer relation, invalid path");
          return ASPAResult::Invalid;
        }
        LOG.debug("It's the first peer relation");
        seenPeer = true;
      }

      if (auth == ASPAAuthResult::NotProviderPlus) {
        LOG.debug("NotProviderPlus relation found, invalid path");
        return ASPAResult::Invalid;
      }

      if (auth == ASPAAuthResult::NoAttestation) {
        hasNoAttestation = true;
      }
    }
  } 

  else if (relation == Relation::Provider) {
    LOG.debug("Performing downstream path verification");

    // Provider path verification
    size_t u_min = route.path.size();
    size_t v_max = 0;
    
    // Find u_min (first NotProviderPlus in forward direction)
    for (size_t i = 0; i < route.path.size() - 1; i++) {
      Router *curr_asys = route.path[i];
      Router *next_asys = route.path[i + 1];
      LOG.debug("Checking authorization between " + std::to_string(curr_asys->ASNumber) + " and " 
                + std::to_string(next_asys->ASNumber));

      ASPAAuthResult auth = checkAuth(curr_asys, next_asys);
      LOG.debug("Authorization result: " + std::to_string(static_cast<int>(auth)));
      if (auth == ASPAAuthResult::NotProviderPlus) {
        LOG.debug("NotProviderPlus relation found, setting u_min");
        u_min = i + 2;
        break;
      }
      if (auth == ASPAAuthResult::NoAttestation) {
        LOG.debug("NoAttestation found");
        hasNoAttestation = true;
      }
    }

    // Find v_max (first NotProviderPlus in reverse direction)
    for (size_t i = route.path.size() - 1; i > 0; i--) {
      Router *curr_asys = route.path[i];
      Router *prev_asys = route.path[i - 1];
      LOG.debug("Checking authorization between " + std::to_string(curr_asys->ASNumber) + " and " 
                + std::to_string(prev_asys->ASNumber));

      ASPAAuthResult auth = checkAuth(curr_asys, prev_asys);
      if (auth == ASPAAuthResult::NotProviderPlus) {
        LOG.debug("NotProviderPlus relation found, setting v_max");
        v_max = i;
        break;
      }
      if (auth == ASPAAuthResult::NoAttestation) {
        LOG.debug("NoAttestation found");
        hasNoAttestation = true;
      }
    }

    if (u_min <= v_max) {
      LOG.debug("u_min: " + std::to_string(u_min) + " v_max: " + std::to_string(v_max));
      return ASPAResult::Invalid;
    }
  } else {
    LOG.error("Unknown relation between final and first hop");
    return ASPAResult::Unknown;
  }

  // Final result determination
  if (hasNoAttestation) {
    LOG.debug("No attestation found, unknown path");
    return ASPAResult::Unknown;
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

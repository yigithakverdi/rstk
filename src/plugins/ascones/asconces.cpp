#include "plugins/ascones/ascones.hpp"
#include <algorithm>

ASConesPolicyEngine::ASConesPolicyEngine(std::shared_ptr<RPKI> rpki)
    : rpki_(rpki) {}

bool ASConesPolicyEngine::shouldAcceptRoute(const Route &route) const {
  ASConesResult result = performASConesVerification(route);
  return result != ASConesResult::Invalid;
}

bool ASConesPolicyEngine::shouldPreferRoute(const Route &currentRoute,
                                            const Route &newRoute) const {
  // Similar preference logic to ASPA
  return false;
}

bool ASConesPolicyEngine::canForwardRoute(Relation sourceRelation,
                                          Relation targetRelation) const {
  return true; // Simplified for now
}

ASConesResult
ASConesPolicyEngine::performASConesVerification(const Route &route) const {
  if (route.path.size() < 2) {
    return ASConesResult::Invalid;
  }

  Router *final = route.path.back();
  Router *firstHop = route.path[route.path.size() - 2];

  if (!final || route.path.empty()) {
    return ASConesResult::Invalid;
  }

  Relation relation = final->GetRelation(firstHop);

  switch (relation) {
  case Relation::Customer:
  case Relation::Peer:
    return upstreamPathVerification(route);
  case Relation::Provider:
    return downstreamPathVerification(route);
  default:
    return ASConesResult::Invalid;
  }
}

bool ASConesPolicyEngine::isCustomerAuthorized(Router *currAS,
                                               Router *nextAS) const {
  auto uspasIt = rpki_->USPAS.find(nextAS->ASNumber);
  if (uspasIt == rpki_->USPAS.end()) {
    return false;
  }

  const auto &customers =
      uspasIt->second
          .providerASes; // In AS-Cones, we treat providerASes as customers
  return std::find(customers.begin(), customers.end(), currAS->ASNumber) !=
         customers.end();
}

ASConesResult
ASConesPolicyEngine::upstreamPathVerification(const Route &route) const {
  if (route.path.size() < 2) {
    return ASConesResult::Invalid;
  }

  if (route.path.size() == 2) {
    return ASConesResult::Valid;
  }

  ASConesResult result = ASConesResult::Valid;
  for (size_t i = 0; i < route.path.size() - 1; i++) {
    Router *currAS = route.path[i];
    Router *nextAS = route.path[i + 1];

    if (!isCustomerAuthorized(currAS, nextAS)) {
      if (rpki_->USPAS.find(nextAS->ASNumber) == rpki_->USPAS.end()) {
        result = ASConesResult::Unknown;
      } else {
        return ASConesResult::Invalid;
      }
    }
  }

  return result;
}

ASConesResult
ASConesPolicyEngine::downstreamPathVerification(const Route &route) const {
  if (route.path.size() < 2) {
    return ASConesResult::Invalid;
  }

  if (route.path.size() <= 3) {
    return ASConesResult::Valid;
  }

  // Find u_min and v_max similar to the Python implementation
  size_t u_min = route.path.size();
  size_t v_max = 0;

  // Calculate u_min
  for (size_t i = 0; i < route.path.size() - 1; i++) {
    Router *currAS = route.path[i];
    Router *nextAS = route.path[i + 1];

    if (!isCustomerAuthorized(currAS, nextAS)) {
      u_min = i + 2;
      break;
    }
  }

  // Calculate v_max
  for (size_t i = route.path.size() - 1; i > 0; i--) {
    Router *currAS = route.path[i];
    Router *prevAS = route.path[i - 1];

    if (!isCustomerAuthorized(currAS, prevAS)) {
      v_max = i;
      break;
    }
  }

  if (u_min <= v_max) {
    return ASConesResult::Invalid;
  }

  // Further verification similar to Python implementation
  size_t k = 1;
  size_t l = route.path.size() - 1;

  // Calculate k and l
  // ... Similar logic to find k and l as in Python ...

  if (static_cast<int>(l) - static_cast<int>(k) <= 1) {
    return ASConesResult::Valid;
  }

  return ASConesResult::Unknown;
}

// ASConesProtocol Implementation
ASConesProtocol::ASConesProtocol(std::shared_ptr<RPKI> rpki)
    : Protocol(std::make_unique<ASConesPolicyEngine>(rpki)), rpki_(rpki) {}

std::string ASConesProtocol::getProtocolName() const { return "AS-Cones"; }

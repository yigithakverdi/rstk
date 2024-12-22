#include "plugins/plugins.hpp"

Protocol::Protocol(std::unique_ptr<PolicyEngine> policyEngine)
    : policyEngine_(std::move(policyEngine)) {}

bool Protocol::acceptRoute(const Route &route) const {
  return policyEngine_->shouldAcceptRoute(route);
}

bool Protocol::preferRoute(const Route &currentRoute,
                           const Route &newRoute) const {
  return policyEngine_->shouldPreferRoute(currentRoute, newRoute);
}

bool Protocol::canForwardTo(Relation sourceRelation,
                            Relation targetRelation) const {
  return policyEngine_->canForwardRoute(sourceRelation, targetRelation);
}

const PolicyEngine &Protocol::getPolicyEngine() const { return *policyEngine_; }

#include "proto/base/base.hpp"
#include "proto/policy.hpp"
#include "proto/proto.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <iostream>

BasePolicy::BasePolicy() { initRules(); };
bool BasePolicy::validate(const route &r) const {
  if (r.hasCycle()) {
    return false;
  }
  return true;
}

bool BasePolicy::prefer(const route &current, const route &newRoute) const {
  if (current.getDestination() != newRoute.getDestination()) {
    return false;
  }

  for (const auto &rule : rules_) {
    int currVal = rule.evaluator(current);
    int newVal = rule.evaluator(newRoute);
    if (currVal == -1 || newVal == -1) {
      continue;
    }
    if (currVal != newVal) {
      return rule.higherIsBetter ? (newVal > currVal) : (newVal < currVal);
    }
  }
  return false;
};

int BasePolicy::localPreference(const route &r) const {
  if (r.getPath().size() < 2) {
    return -1;
  }

  router *final = r.getDestination();
  router *first = r.getPath()[r.getPath().size() - 2];

  relation rel = final->getRelation(first);
  if (rel != relation::unknown) {
    return static_cast<int>(rel);
  }

  return -1;
};

int BasePolicy::asPathLength(const route &r) const { return static_cast<int>(r.getPath().size()); };
int BasePolicy::nextHopASNumber(const route &r) const {
  if (r.getPath().size() < 2) {
    return -1;
  }

  router *nextHop = r.getPath()[r.getPath().size() - 2];
  return nextHop->getNumber();
};

void BasePolicy::initRules() {
  rules_ = {{rules::localPreference, [this](const route &r) { return localPreference(r); }, true},
            {rules::asPathLength, [this](const route &r) { return asPathLength(r); }, false},
            {rules::nextHopASNumber, [this](const route &r) { return nextHopASNumber(r); }, false}};
};

bool BasePolicy::forward(route *r, relation rel) const {
  router *final = r->getPath().back();
  router *first = r->getPath()[r->getPath().size() - 2];

  std::cout << "    Final AS: " << final->getId() << std::endl;
  std::cout << "    First AS: " << first->getId() << std::endl;

  relation firstHopRel = final->getRelation(first);

  std::cout << "    Final AS (" << final->getId() << ")" << " is " << final->toString(firstHopRel)
            << " to First AS (" << first->getId() << ")" << std::endl;
  return firstHopRel == relation::customer || rel == relation::customer;
};

BaseProtocol::BaseProtocol() : IProto(std::make_unique<BasePolicy>()) {}
std::string BaseProtocol::name() const { return "Base"; };
std::string BaseProtocol::info() const { return "Base Protocol"; };

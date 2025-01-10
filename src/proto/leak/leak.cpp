#include "proto/leak/leak.hpp"
#include "router/route.hpp"
#include "router/router.hpp"

LeakPolicy::LeakPolicy() { initRules(); };
bool LeakPolicy::prefer(const route &current, const route &newRoute) const {
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

int LeakPolicy::localPreference(const route &r) const {
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

bool LeakPolicy::validate(const route &r) const { return true; }
int LeakPolicy::asPathLength(const route &r) const { return static_cast<int>(r.getPath().size()); };
int LeakPolicy::nextHopASNumber(const route &r) const {
  if (r.getPath().size() < 2) {
    return -1;
  }

  router *nextHop = r.getPath()[r.getPath().size() - 2];
  return nextHop->getNumber();
};

void LeakPolicy::initRules() {
  rules_ = {{rules::localPreference, [this](const route &r) { return localPreference(r); }, true},
            {rules::asPathLength, [this](const route &r) { return asPathLength(r); }, false},
            {rules::nextHopASNumber, [this](const route &r) { return nextHopASNumber(r); }, false}};
};

bool LeakPolicy::forward(relation source, relation target) const { return true; }
LeakProtocol::LeakProtocol() : IProto(std::make_unique<LeakPolicy>()) {}
std::string LeakProtocol::name() const { return "leak"; }
std::string LeakProtocol::info() const { return "Leak protocol"; }

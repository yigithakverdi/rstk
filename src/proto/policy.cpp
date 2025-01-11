#include "proto/policy.hpp"
#include "router/route.hpp"
#include "router/router.hpp"

IPolicy::~IPolicy() = default;
bool IPolicy::forward(route *r, relation rel) const { return false; }
bool IPolicy::validate(const route &r) const { return !r.hasCycle(); }
bool IPolicy::prefer(const route &r1, const route &r2) const {
  return r1.getPath().size() < r2.getPath().size();
}


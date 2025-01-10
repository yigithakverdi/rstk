#include "router/route.hpp"
#include "router/router.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

route::route() : destination(nullptr), authenticated(false), originValid(false), pathEndValid(false) {}
route::~route() { path.clear(); }

bool route::hasCycle() const {
  std::unordered_set<router *> visited;
  for (router *r : path) {
    if (visited.find(r) != visited.end()) {
      return true;
    }
    visited.insert(r);
  }
  return false;
}

void route::reversePath() { std::reverse(path.begin(), path.end()); }
std::string route::toString() {
  std::stringstream ss;
  ss << "Route to " << destination->getId() << ":\n";
  for (router *r : path) {
    ss << "  " << r->getId() << "\n";
  }
  return ss.str();
}

std::string route::toPathString() {
  std::stringstream ss;
  for (router *r : path) {
    ss << r->getId() << " ";
  }
  return ss.str();
}

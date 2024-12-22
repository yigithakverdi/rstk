#include "router/route.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

Route::Route() : destination(nullptr), path() {}

// If a rounter contains cycle return ture else return false, a router
// contains cycle when there is duplicate router in the path
bool Route::ContainsCycle() const {
  // Create a set to track unique routers in the path
  std::unordered_set<Router *> visited;

  // Iterate through the path
  for (const auto &router : path) {
    // If this router is already in the visited set, we've found a cycle
    if (visited.count(router) > 0) {
      return true;
    }

    // Add the router to the visited set
    visited.insert(router);
  }

  // No cycles found
  return false;
}

void Route::ReversePath() { std::reverse(path.begin(), path.end()); }

std::string Route::ToString() const {
  std::stringstream ss;
  ss << "Route to AS" << (destination ? std::to_string(destination->ASNumber) : "None");
  ss << " Path: [";

  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i]) {
      ss << "AS" << path[i]->ASNumber;
      if (i < path.size() - 1) {
        ss << " -> ";
      }
    } else {
      ss << "None";
      if (i < path.size() - 1) {
        ss << " -> ";
      }
    }
  }
  ss << "]";

  ss << " Flags: {";
  ss << "authenticated=" << (authenticated ? "true" : "false");
  ss << ", originValid=" << (originValid ? "true" : "false");
  ss << ", pathEndInvalid=" << (pathEndInvalid ? "true" : "false");
  ss << "}";

  return ss.str();
}

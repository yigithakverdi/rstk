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
  ss << "\n  Destination: AS" << (destination ? std::to_string(destination->ASNumber) : "None");

  // Path information
  ss << "\n  Path: [";
  // Iterate in reverse to show forward path
  for (int i = path.size() - 1; i >= 0; --i) {
    if (path[i]) {
      ss << "AS" << path[i]->ASNumber;
      // Show relationship between consecutive ASes
      if (i > 0 && path[i - 1]) {                         // Changed condition to check previous AS
        Relation rel = path[i]->GetRelation(path[i - 1]); // Get relation to previous AS
        ss << " -(" << relationToString(rel) << ")-> ";
      }
    } else {
      ss << "None";
    }
  }
  ss << "]";

  // Protocol validation flags
  ss << "\n  Protocol Status:";
  ss << "\n    - Authenticated: " << (authenticated ? "✓" : "✗");
  ss << "\n    - Origin Valid: " << (originValid ? "✓" : "✗");
  ss << "\n    - Path End Valid: " << (!pathEndInvalid ? "✓" : "✗");

  // Check for valley-free violations
  bool hasValleyViolation = false;
  std::string violationDetails;
  for (size_t i = 0; i < path.size() - 1; i++) {
    if (path[i] && path[i + 1]) {
      Relation rel = path[i]->GetRelation(path[i + 1]);
      if (rel == Relation::Provider && i > 0) {
        hasValleyViolation = true;
        violationDetails +=
            "\n      - Customer-to-Provider transition after index " + std::to_string(i);
      }
    }
  }

  if (hasValleyViolation) {
    ss << "\n  Valley-Free Violations:" << violationDetails;
  } else {
    ss << "\n  Valley-Free: ✓ Valid";
  }

  return ss.str();
}

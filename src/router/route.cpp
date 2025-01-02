#include "router/route.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

Route::Route() : destination(nullptr), path() {}

struct PathSegment {
  Router *from;
  Router *to;
  Relation relation;
};

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
  // Iterate normally to show forward path
  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i]) {
      ss << "AS" << path[i]->ASNumber;
      // Show relationship between consecutive ASes
      if (i < path.size() - 1 && path[i + 1]) {           // Changed condition to check next AS
        Relation rel = path[i]->GetRelation(path[i + 1]); // Get relation to next AS
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

  bool hasValleyViolation = false;
  std::string violationDetails;

  // First get the path segments for easier analysis
  std::vector<PathSegment> segments;
  for (size_t i = 0; i < path.size() - 1; i++) {
    if (path[i] && path[i + 1]) {
      segments.push_back({path[i], path[i + 1], path[i]->GetRelation(path[i + 1])});
    }
  }

  // For each segment, check if traffic propagation follows valley-free rules
  for (size_t i = 0; i < segments.size(); i++) {
    if (i == 0)
      continue; // Skip first segment (originating AS)

    // Get current and previous relations
    Relation prevRelation = segments[i - 1].relation;
    Relation currentRelation = segments[i].relation;

    // Rule 1: If received from customer, can go anywhere
    if (prevRelation == Relation::Customer) {
      continue; // Always valid
    }

    // Rule 2: If received from peer, must go to customer
    else if (prevRelation == Relation::Peer) {
      if (currentRelation != Relation::Customer) {
        hasValleyViolation = true;
        violationDetails += "\n      - Peer traffic sent to non-customer at AS" +
                            std::to_string(segments[i].from->ASNumber);
      }
    }

    // Rule 3: If received from provider, must go to customer
    else if (prevRelation == Relation::Provider) {
      if (currentRelation != Relation::Customer) {
        hasValleyViolation = true;
        violationDetails += "\n      - Provider traffic sent to non-customer at AS" +
                            std::to_string(segments[i].from->ASNumber);
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

std::string Route::PathToString() const {
  std::stringstream ss;
  ss << "Path: [";
  // Iterate normally to show forward path
  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i]) {
      ss << "AS" << path[i]->ASNumber;
      // Show relationship between consecutive ASes
      if (i < path.size() - 1 && path[i + 1]) {           // Check next AS
        Relation rel = path[i]->GetRelation(path[i + 1]); // Get relation to next AS
        ss << " -(" << relationToString(rel) << ")-> ";
      }
    } else {
      ss << "None";
    }
  }
  ss << "]";
  return ss.str();
}

#include "cli/interactive.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
#include <unordered_set>

// Forward-declare any helper functions you need (or #include them if defined elsewhere)
// e.g. getCharChoice, clearScreen, validatePath, getValidNextHops, etc.

// Example placeholders for demonstration:
static void clearScreen() { std::cout << "\033[2J\033[1;1H"; }
static char getCharChoice(const std::string &prompt) {
  std::cout << prompt;
  char c{};
  std::cin >> c;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  return static_cast<char>(std::tolower(c));
}

// Helper function to validate entire path
bool validatePath(const Route &path) {
  if (path.path.empty())
    return true;

  // Check for cycles
  if (path.ContainsCycle())
    return false;

  // Validate against each router's protocol along the path
  for (size_t i = 0; i < path.path.size() - 1; i++) {
    Router *current = path.path[i];
    Router *next = path.path[i + 1];

    // Create a temporary route for this segment
    Route segment;
    segment.destination = path.destination;
    segment.path = std::vector<Router *>(path.path.begin(), path.path.begin() + i + 2);

    // Check if this router would accept this route
    if (!current->proto->acceptRoute(segment)) {
      return false;
    }

    // Check relationship policies
    Relation sourceRel = current->GetRelation(next);
    if (i > 0) {
      Router *prev = path.path[i - 1];
      Relation targetRel = prev ? current->GetRelation(prev) : Relation::Unknown;
      if (!current->proto->canForwardTo(sourceRel, targetRel)) {
        return false;
      }
    }
  }

  return true;
}

// Helper function to get valid next hops based on protocol policies
std::vector<std::shared_ptr<Router>> getValidNextHops(std::shared_ptr<Topology> topology,
                                                      std::shared_ptr<Router> current_router,
                                                      const Route &current_path) {
  if (!current_router)
    return {};

  std::vector<std::shared_ptr<Router>> valid_hops;

  // Get all neighbors
  for (const auto &[asNum, neighbor] : current_router->neighbors_) {
    // Create a temporary route to test this path
    Route test_path = current_path;
    test_path.path.push_back(neighbor.router);

    // Check if this would be a valid route according to the router's protocol
    if (current_router->proto->acceptRoute(test_path)) {
      valid_hops.push_back(topology->GetRouter(asNum));
    }
  }

  return valid_hops;
}

CommandResult interactiveRouterExplorer(std::shared_ptr<Topology> topology,
                                        std::shared_ptr<Router> start_router) {
  std::vector<std::shared_ptr<Router>> navigation_stack;
  std::shared_ptr<Router> current_router = start_router;
  Route current_path; // Keep track of the actual route being built

  while (true) {
    clearScreen();

    // Show breadcrumb with protocol validation status
    std::cout << BLUE << "Current Path: ";
    if (navigation_stack.empty()) {
      std::cout << "root";
    } else {
      for (size_t i = 0; i < navigation_stack.size(); i++) {
        std::cout << "AS" << navigation_stack[i]->ASNumber;
        if (i < navigation_stack.size() - 1)
          std::cout << " → ";
      }
    }
    if (current_router) {
      std::cout << " → " << "AS" << current_router->ASNumber;
    }
    std::cout << RESET << "\n";

    // Show path validation status if we have a path
    if (!navigation_stack.empty()) {
      std::cout << "Path Validation: ";
      bool isValid = validatePath(current_path);
      if (isValid) {
        std::cout << GREEN << "✓ Valid Path" << RESET << "\n";
      } else {
        std::cout << RED << "✗ Invalid Path" << RESET << "\n";
      }
    }
    std::cout << "\n";

    // Show current router details
    if (current_router) {
      std::cout << CYAN << "Current Router: AS" << current_router->ASNumber << RESET << "\n";
      std::cout << "Protocol: " << current_router->proto->getProtocolName() << "\n";
      std::cout << "Tier: " << current_router->Tier << "\n\n";

      // Get valid next hops based on protocol
      auto validNextHops = getValidNextHops(topology, current_router, current_path);

      if (validNextHops.empty()) {
        std::cout << YELLOW << "No valid next hops available based on current protocol policies."
                  << RESET << "\n";
      } else {
        std::cout << "Valid next hops:\n";
        for (size_t i = 0; i < validNextHops.size(); i++) {
          auto &next = validNextHops[i];
          std::cout << i + 1 << ". AS" << next->ASNumber << " ("
                    << relationToString(current_router->GetRelation(next.get())) << ")\n";
        }
      }
    }

    std::cout << "\nCommands:\n";
    std::cout << "b. Go back\n";
    std::cout << "q. Quit explorer\n";
    std::cout << "r. Reset path\n\n";

    char choice = getCharChoice("Enter your choice: ");

    if (choice == 'q')
      break;
    if (choice == 'r') {
      navigation_stack.clear();
      current_router = nullptr;
      current_path = Route();
      continue;
    }
    if (choice == 'b') {
      if (!navigation_stack.empty()) {
        current_router = navigation_stack.back();
        navigation_stack.pop_back();
        current_path.path.pop_back();
      }
      continue;
    }

    int idx = choice - '1';
    auto validNextHops = getValidNextHops(topology, current_router, current_path);
    if (idx >= 0 && idx < static_cast<int>(validNextHops.size())) {
      auto next_router = validNextHops[idx];

      // Update path
      if (current_router) {
        navigation_stack.push_back(current_router);
      }
      current_router = next_router;

      // Update route path
      if (current_path.path.empty()) {
        current_path.destination = current_router.get();
      }
      current_path.path.push_back(current_router.get());
    }
  }

  return CommandResult("Explorer closed");
}

// If you have a function for relationToString(Relation), define or include it
static std::string localRelationToString(Relation r) {
  switch (r) {
  case Relation::Provider:
    return "Provider";
  case Relation::Customer:
    return "Customer";
  case Relation::Peer:
    return "Peer";
  default:
    return "Unknown";
  }
}

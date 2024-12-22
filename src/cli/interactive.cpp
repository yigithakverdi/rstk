#include "cli/interactive.hpp"
#include "router/router.hpp"
#include "router/route.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>

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

// Your domain-specific placeholders:
static bool validatePath(const Route &path) {
  // Insert your logic or #include from wherever it is defined
  return true;
}
static std::vector<std::shared_ptr<Router>>
getValidNextHops(std::shared_ptr<Topology> /*topology*/, std::shared_ptr<Router> /*current_router*/,
                 const Route & /*current_path*/) {
  // Insert your logic or #include from wherever it is defined
  return {};
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

CommandResult interactiveRouterExplorer(std::shared_ptr<Topology> topology,
                                        std::shared_ptr<Router> start_router) {
  std::vector<std::shared_ptr<Router>> navigation_stack;
  std::shared_ptr<Router> current_router = start_router;
  Route current_path; // Keep track of the actual route being built

  while (true) {
    clearScreen();

    // 1) Show breadcrumb with protocol validation status
    std::cout << BLUE << "Current Path: ";
    if (navigation_stack.empty()) {
      std::cout << "root";
    } else {
      for (size_t i = 0; i < navigation_stack.size(); i++) {
        std::cout << "AS" << navigation_stack[i]->ASNumber;
        if (i < navigation_stack.size() - 1) {
          std::cout << " → ";
        }
      }
    }
    if (current_router) {
      std::cout << " → " << "AS" << current_router->ASNumber;
    }
    std::cout << RESET << "\n";

    // 2) Show path validation status
    if (!navigation_stack.empty()) {
      bool isValid = validatePath(current_path);
      std::cout << "Path Validation: ";
      if (isValid) {
        std::cout << GREEN << "✓ Valid Path" << RESET << "\n";
      } else {
        std::cout << RED << "✗ Invalid Path" << RESET << "\n";
      }
    }
    std::cout << "\n";

    // 3) Show current router details
    if (current_router) {
      std::cout << CYAN << "Current Router: AS" << current_router->ASNumber << RESET << "\n";
      std::cout << "Protocol: " << current_router->proto->getProtocolName() << "\n";
      std::cout << "Tier: " << current_router->Tier << "\n\n";

      // 4) Get valid next hops based on the router’s protocol
      auto validNextHops = getValidNextHops(topology, current_router, current_path);

      if (validNextHops.empty()) {
        std::cout << YELLOW << "No valid next hops available based on current protocol policies."
                  << RESET << "\n";
      } else {
        std::cout << "Valid next hops:\n";
        for (size_t i = 0; i < validNextHops.size(); i++) {
          auto &next = validNextHops[i];
          std::cout << (i + 1) << ". AS" << next->ASNumber << " ("
                    << localRelationToString(current_router->GetRelation(next.get())) << ")\n";
        }
      }
    }

    // 5) Menu
    std::cout << "\nCommands:\n";
    std::cout << "b. Go back\n";
    std::cout << "q. Quit explorer\n";
    std::cout << "r. Reset path\n\n";

    // 6) Get user choice
    char choice = getCharChoice("Enter your choice: ");
    if (choice == 'q') {
      break;
    }
    if (choice == 'r') {
      navigation_stack.clear();
      current_router.reset();
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

    // 7) If user typed a digit for next hops
    int index = choice - '1';
    auto validNextHops = getValidNextHops(topology, current_router, current_path);
    if (index >= 0 && index < static_cast<int>(validNextHops.size())) {
      auto next_router = validNextHops[index];

      // Push the old current onto the stack so we can go "b" back
      if (current_router) {
        navigation_stack.push_back(current_router);
      }
      current_router = next_router;

      // If it’s the first router in the path, set it as destination
      if (current_path.path.empty()) {
        current_path.destination = current_router.get();
      }
      current_path.path.push_back(current_router.get());
    }
  }

  return CommandResult("Explorer closed");
}

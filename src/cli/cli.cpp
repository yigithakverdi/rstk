#include "cli/cli.hpp"
#include "cli/commands.hpp"
#include "cli/ui.hpp"
#include <iostream>
#include <readline/chardefs.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <variant>

CLI::CLI() : running_(false) {
  registerEngineCallbacks();
  registerAllCommands(*this);
}

void CLI::run() {
  running_ = true;
  printInfo("Welcome to BGP Simulator CLI");
  printInfo("Type 'help' for available commands");

  char *input;
  while (running_ && (input = readline("rstk> ")) != nullptr) {
    std::string command(input);
    free(input);

    if (!command.empty()) {
      add_history(command.c_str());
      state_.addToHistory(command);
      try {
        auto result = processCommand(command);
        if (std::holds_alternative<std::string>(result.result)) {
          printSuccess(std::get<std::string>(result.result));
        }
      } catch (const std::exception &e) {
        printError(e.what());
      }
    }
  }
}

void CLI::stop() { running_ = false; }

CommandResult CLI::processCommand(const std::string &input) {
  auto args = parseCommandLine(input);
  if (args.empty()) {
    return CommandResult(false);
  }

  std::string command_name = args[0];
  args.erase(args.begin());

  auto it = commands_.find(command_name);
  if (it == commands_.end()) {
    throw std::runtime_error("Unknown command: " + command_name);
  }

  return it->second.handler(args, state_);
}

std::vector<std::string> CLI::parseCommandLine(const std::string &input) {
  std::vector<std::string> tokens;
  std::istringstream iss(input);
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

void CLI::printSuccess(const std::string &message) {
  std::cout << GREEN << "[OK] " << message << RESET << std::endl;
}

void CLI::printInfo(const std::string &message) {
  std::cout << BLUE << "[INFO] " << message << RESET << std::endl;
}

void CLI::printWarning(const std::string &message) {
  std::cout << YELLOW << "[WARNING] " << message << RESET << std::endl;
}

void CLI::printError(const std::string &message) {
  std::cout << RED << "[ERROR] " << message << RESET << std::endl;
}
void CLI::unregisterCommand(const std::string &name) { commands_.erase(name); }

void CLI::registerCommand(const std::string &name, const Command &command) {
  commands_[name] = command;
}

void CLI::registerEngineCallbacks() {
  state_.getEngine().registerEventCallback(
      [this](ExperimentEvent event, const std::string &details) {
        handleExperimentEvent(event, details);
      });
}

// Helper function to get protocol statistics
std::string CLI::getProtocolStatistics(const std::shared_ptr<Topology> &topology) {
  std::unordered_map<std::string, int> protocol_counts;
  size_t total_routers_with_protocols = 0;

  // Count protocols
  for (const auto &[id, router] : topology->G->nodes) {
    if (router->proto) {
      protocol_counts[router->proto->getProtocolName()]++;
      total_routers_with_protocols++;
    }
  }

  // Calculate percentages and format output
  std::stringstream ss;
  ss << "\nProtocol Distribution:\n";
  for (const auto &[proto_name, count] : protocol_counts) {
    double percentage = (count * 100.0) / topology->G->nodes.size();
    ss << "  " << std::left << std::setw(20) << proto_name << ": " << std::setw(6) << count
       << " ASes "
       << "(" << std::fixed << std::setprecision(2) << percentage << "%)\n";
  }

  // Add coverage information
  double coverage = (total_routers_with_protocols * 100.0) / topology->G->nodes.size();
  ss << "\nProtocol Coverage: " << std::fixed << std::setprecision(2) << coverage
     << "% of ASes have protocols assigned\n";

  return ss.str();
}

CommandResult CLI::handleExperimentEvent(ExperimentEvent event, const std::string &details) {
  switch (event) {
  case ExperimentEvent::STARTED:
    printInfo("Experiment started: " + details);
    return CommandResult("Experiment started");

  case ExperimentEvent::TRIAL_STARTED:
    printInfo("Trial started: " + details);
    return CommandResult("Trial started");

  case ExperimentEvent::TRIAL_COMPLETED:
    // Update progress bar here if needed
    printInfo("Trial completed: " + details);
    return CommandResult("Trial completed");

  case ExperimentEvent::PAUSED:
    printWarning("Experiment paused: " + details);
    return CommandResult("Experiment paused");

  case ExperimentEvent::RESUMED:
    printInfo("Experiment resumed: " + details);
    return CommandResult("Experiment resumed");

  case ExperimentEvent::COMPLETED:
    printSuccess("Experiment completed: " + details);
    return CommandResult("Experiment completed");

  case ExperimentEvent::ERROR:
    printError("Experiment error: " + details);
    return CommandResult("Experiment error");

  default:
    // Handle any other unexpected events (or ignore them)
    return CommandResult("");
  }
}

CommandResult CLI::handleHelp(const std::vector<std::string> &args) {
  if (args.empty()) {
    std::stringstream ss;
    ss << "Available commands:\n";
    for (const auto &[name, cmd] : commands_) {
      ss << "  " << CYAN << name << RESET << " - " << cmd.description << "\n";
      ss << "    Usage: ";
      for (const auto &usage : cmd.usage) {
        ss << "\n      " << BLUE << usage << RESET;
      }
      ss << "\n\n";
    }
    return CommandResult(ss.str());
  }

  auto it = commands_.find(args[0]);
  if (it == commands_.end()) {
    throw std::runtime_error("Unknown command: " + args[0]);
  }

  std::stringstream ss;
  ss << CYAN << it->first << RESET << ": " << it->second.description << "\n\n";
  ss << "Usage:\n";
  for (const auto &usage : it->second.usage) {
    ss << "  " << BLUE << usage << RESET << "\n";
  }
  return CommandResult(ss.str());
}

// Example load topology command handler
CommandResult CLI::handleLoadTopology(const std::vector<std::string> &args, CLIState &state) {
  if (args.empty()) {
    throw std::runtime_error("No file specified");
  }

  // Create and start spinner
  Spinner spinner;
  spinner.start();
  std::cout << "\n"; // Add some spacing

  bool success = false;
  std::string message;

  try {
    // Show loading message with spinner
    while (!success) {
      spinner.update();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      if (state.getEngine().loadTopology(args[0])) {
        success = true;

        // Get the topology to show relationship count
        auto topology = state.getEngine().getTopology();
        size_t relationCount = topology->G->nodes.size();

        message = "Loaded topology from " + args[0] + "\nTopology has " +
                  std::to_string(relationCount) + " relationships";
        break;
      }
    }
  } catch (const std::exception &e) {
    spinner.stop();
    throw std::runtime_error(e.what());
  }

  // Stop spinner and clear its line
  spinner.stop();
  std::cout << "\033[1A"; // Move cursor up one line
  std::cout << "\033[2K"; // Clear the line

  if (!success) {
    throw std::runtime_error(state.getEngine().getLastError());
  }

  return CommandResult(message);
}

// Example run experiment command handler
CommandResult CLI::handleRunExperiment(const std::vector<std::string> &args, CLIState &state) {
  if (args.size() < 2) {
    throw std::runtime_error("Insufficient arguments");
  }

  std::string exp_type = args[0];
  std::vector<std::string> params(args.begin() + 1, args.end());

  if (!state.getEngine().startExperiment(exp_type, params)) {
    throw std::runtime_error(state.getEngine().getLastError());
  }

  return CommandResult("Started experiment: " + exp_type);
}

CommandResult CLI::handleExit(const std::vector<std::string> &args, CLIState &state) {
  state.getEngine().stopExperiment();
  stop();
  return CommandResult("Exiting CLI");
}

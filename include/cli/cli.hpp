#pragma once

#include "cli/state.hpp" // Assume this header exists or will be created separately
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

// ANSI color codes
const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";

// Simple wrapper for command results
struct CommandResult {
  std::variant<bool, std::string> result;
  explicit CommandResult(const std::variant<bool, std::string> &r) : result(r) {}
};

// Command structure
struct Command {
  std::string name;
  std::string description;
  std::vector<std::string> usage;
  std::function<CommandResult(const std::vector<std::string> &, CLIState &)> handler;
};

class CLI {
public:
  CLI();
  void run();
  void stop();
  void registerCommand(const std::string &name, const Command &command);

  CommandResult handleHelp(const std::vector<std::string> &args);
  CommandResult handleExit(const std::vector<std::string> &args, CLIState &state);
  CommandResult handleLoadTopology(const std::vector<std::string> &args, CLIState &state);
  CommandResult handleRunExperiment(const std::vector<std::string> &args, CLIState &state);
  CommandResult handleExperimentEvent(ExperimentEvent event, const std::string &details);

  std::string getProtocolStatistics(const std::shared_ptr<Topology> &topology);
  void registerEngineCallbacks();

private:
  bool running_;
  CLIState state_;
  std::map<std::string, Command> commands_;

  // Process the given input string and run the matching command
  CommandResult processCommand(const std::string &input);

  // Parse a command line into tokens
  std::vector<std::string> parseCommandLine(const std::string &input);

  // Utility printing methods
  void printError(const std::string &message);
  void printSuccess(const std::string &message);
  void printInfo(const std::string &message);
  void printWarning(const std::string &message);

protected:
  // Make these protected if you want derived classes to customize
  void unregisterCommand(const std::string &name);
};

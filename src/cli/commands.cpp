#include "cli/commands.hpp"
#include "cli/cli.hpp"
#include "cli/interactive.hpp"
#include "engine/experiments/register.hpp"
#include "plugins/base/base.hpp"
#include <iomanip>

void registerAllCommands(CLI &cli) {
  cli.registerCommand("help", {"help",
                               "Displays help information for available commands",
                               {"help", "help <command>"},
                               [&](const std::vector<std::string> &args, CLIState &state) {
                                 return cli.handleHelp(args);
                               }});

  cli.registerCommand("load", {"load",
                               "Load a topology from a file",
                               {"load <filename>"},
                               [&cli](const std::vector<std::string> &args,
                                      CLIState &state) { // Note the cli capture
                                 return cli.handleLoadTopology(args, state);
                               }});

  cli.registerCommand("clear", {"clear",
                                "Clear the terminal screen",
                                {"clear"},
                                [](const std::vector<std::string> &args, CLIState &state) {
                                  // ANSI escape code to clear screen and move cursor to top-left
                                  std::cout << "\033[2J\033[H" << std::flush;
                                  return CommandResult("");
                                }});

  cli.registerCommand("exit", {"exit",
                               "Exit the CLI",
                               {"exit"},
                               [&](const std::vector<std::string> &args, CLIState &state) {
                                 return cli.handleExit(args, state);
                               }});

  cli.registerCommand("find", {"find",
                               "Find and analyze routes to a target AS",
                               {"find <target-AS>"},
                               [](const std::vector<std::string> &args, CLIState &state) {
                                 if (args.empty()) {
                                   throw std::runtime_error("Target AS number required");
                                 }

                                 auto topology = state.getEngine().getTopology();
                                 if (!topology) {
                                   throw std::runtime_error(state.getEngine().getLastError());
                                 }

                                 try {
                                   int targetAS = std::stoi(args[0]);
                                   auto router = topology->GetRouter(targetAS);

                                   if (!router) {
                                     throw std::runtime_error("Router not found: AS" + args[0]);
                                   }

                                   // Use Engine's deployment strategy instead of direct flag
                                   // setting
                                   if (!state.getEngine().setDeploymentStrategy(
                                           std::make_unique<BaseDeploymentStrategy>())) {
                                     throw std::runtime_error(state.getEngine().getLastError());
                                   }

                                   topology->setDeploymentTrue();
                                   topology->FindRoutesTo(router.get(), VerbosityLevel::NORMAL);
                                   return CommandResult("Route analysis complete");

                                 } catch (const std::invalid_argument &e) {
                                   throw std::runtime_error("Invalid AS number format");
                                 }
                               }});

  cli.registerCommand("hijack",
                      {"hijack",
                       "Simulate a route hijack attack",
                       {"hijack <victim-AS> <attacker-AS> <path-length>"},
                       [](const std::vector<std::string> &args, CLIState &state) {
                         if (args.size() < 3) {
                           throw std::runtime_error("Required: victim-AS attacker-AS path-length");
                         }

                         auto topology = state.getEngine().getTopology();
                         if (!topology) {
                           throw std::runtime_error(state.getEngine().getLastError());
                         }

                         try {
                           int victimAS = std::stoi(args[0]);
                           int attackerAS = std::stoi(args[1]);
                           int pathLength = std::stoi(args[2]);

                           auto victim = topology->GetRouter(victimAS);
                           auto attacker = topology->GetRouter(attackerAS);

                           if (!victim || !attacker) {
                             throw std::runtime_error("Router not found");
                           }

                           // Use Engine's deployment strategy instead of direct topology
                           // manipulation
                           if (!state.getEngine().setDeploymentStrategy(
                                   std::make_unique<BaseDeploymentStrategy>())) {
                             throw std::runtime_error(state.getEngine().getLastError());
                           }

                           topology->setDeploymentTrue();
                           topology->Hijack(victim.get(), attacker.get(), pathLength,
                                            VerbosityLevel::NORMAL);
                           return CommandResult("Hijack simulation complete");

                         } catch (const std::invalid_argument &e) {
                           throw std::runtime_error("Invalid AS number or path length format");
                         }
                       }});

  // Updated topology-info command
  cli.registerCommand("topology-info",
                      {"topology-info",
                       "Display detailed information about current topology",
                       {"topology-info", "topology-info --verbose|-v"},
                       [&cli](const std::vector<std::string> &args, CLIState &state) {
                         auto topology = state.getEngine().getTopology();
                         if (!topology) {
                           throw std::runtime_error(state.getEngine().getLastError());
                         }

                         bool verbose =
                             (args.size() > 0 && (args[0] == "--verbose" || args[0] == "-v"));

                         std::stringstream ss;
                         ss << "Topology Statistics:\n";
                         ss << "==================\n";

                         // Basic topology info
                         ss << "Total ASes: " << topology->G->nodes.size() << "\n";

                         // Tier information
                         auto tier1 = topology->GetTierOne();
                         auto tier2 = topology->GetTierTwo();
                         auto tier3 = topology->GetTierThree();

                         ss << "\nTier Distribution:\n";
                         ss << "  Tier 1 (Transit): " << std::setw(6) << tier1.size() << " ASes ("
                            << std::fixed << std::setprecision(2)
                            << (tier1.size() * 100.0 / topology->G->nodes.size()) << "%)\n";
                         ss << "  Tier 2 (Transit): " << std::setw(6) << tier2.size() << " ASes ("
                            << (tier2.size() * 100.0 / topology->G->nodes.size()) << "%)\n";
                         ss << "  Tier 3 (Stub)   : " << std::setw(6) << tier3.size() << " ASes ("
                            << (tier3.size() * 100.0 / topology->G->nodes.size()) << "%)\n";

                         // Add protocol statistics
                         ss << cli.getProtocolStatistics(topology);

                         if (verbose) {
                           ss << "\nRelationship Statistics:\n";
                           // Add relationship type counts (peer, provider, customer)
                           // Add average number of relationships per AS
                           // Add other verbose statistics that don't require listing individual
                           // routers
                         }

                         return CommandResult(ss.str());
                       }});

  cli.registerCommand("stop-experiment",
                      {"stop-experiment",
                       "Stop the currently running experiment",
                       {"stop-experiment"},
                       [&](const std::vector<std::string> &args, CLIState &state) {
                         if (!state.getEngine().stopExperiment()) {
                           throw std::runtime_error(state.getEngine().getLastError());
                         }
                         return CommandResult("Stopped experiment");
                       }});

  cli.registerCommand("pause-experiment",
                      {"pause-experiment",
                       "Pause the currently running experiment",
                       {"pause-experiment"},
                       [&](const std::vector<std::string> &args, CLIState &state) {
                         if (!state.getEngine().pauseExperiment()) {
                           throw std::runtime_error(state.getEngine().getLastError());
                         }
                         return CommandResult("Paused experiment");
                       }});

  cli.registerCommand("resume-experiment",
                      {"resume-experiment",
                       "Resume a paused experiment",
                       {"resume-experiment"},
                       [&](const std::vector<std::string> &args, CLIState &state) {
                         if (!state.getEngine().resumeExperiment()) {
                           throw std::runtime_error(state.getEngine().getLastError());
                         }
                         return CommandResult("Resumed experiment");
                       }});

  cli.registerCommand("list-experiments",
                      {"list-experiments",
                       "List all available experiments",
                       {"list-experiments"},
                       [](const std::vector<std::string> &args, CLIState &state) {
                         std::stringstream ss;
                         ss << "Available experiments:\n";
                         for (const auto &exp : ExperimentRegistry::Instance().listExperiments()) {
                           ss << "\n" << exp.name << ":\n";
                           ss << "  Description: " << exp.description << "\n";
                           ss << "  Parameters:\n";
                           for (const auto &param : exp.parameters) {
                             ss << "    - " << param << "\n";
                           }
                         }
                         return CommandResult(ss.str());
                       }});

  cli.registerCommand("run-experiment",
                      {"run-experiment",
                       "Run a specified experiment",
                       {"run-experiment <experiment-type> [parameters...]"},
                       [](const std::vector<std::string> &args, CLIState &state) {
                         if (args.empty()) {
                           throw std::runtime_error("Experiment type required");
                         }

                         // Get experiment type and parameters
                         std::string exp_type = args[0];
                         std::vector<std::string> exp_args(args.begin() + 1, args.end());

                         // Header display (keep existing display logic)
                         std::stringstream ss;
                         ss << "┌──────────────────────────────────────────┐\n";
                         ss << "│ Running Experiment: " << std::left << std::setw(20) << exp_type
                            << " │\n";
                         ss << "└──────────────────────────────────────────┘\n\n";
                         std::cout << ss.str();

                         // Start experiment through engine
                         if (!state.getEngine().startExperiment(exp_type, exp_args)) {
                           throw std::runtime_error(state.getEngine().getLastError());
                         }

                         // Engine will handle the experiment execution and progress
                         // updates through event callbacks, which will be handled in
                         // handleExperimentEvent

                         return CommandResult("Experiment started successfully");
                       }});

  cli.registerCommand("experiment-status",
                      {"experiment-status",
                       "Show current experiment status",
                       {"experiment-status"},
                       [](const std::vector<std::string> &args, CLIState &state) {
                         if (!state.getEngine().isExperimentRunning()) {
                           return CommandResult("No experiment running");
                         }

                         auto exp_state = state.getEngine().getExperimentState();
                         std::stringstream ss;
                         ss << "Experiment Status:\n";
                         ss << "Type: " << exp_state.type << "\n";
                         ss << "Progress: " << exp_state.progress << "%\n";
                         ss << "Status: " << exp_state.current_status << "\n";
                         ss << "Trials: " << exp_state.completed_trials << "/"
                            << exp_state.total_trials;

                         return CommandResult(ss.str());
                       }});

  cli.registerCommand("show-router",
                      {"show-router",
                       "Show details about a specific router",
                       {"show-router <AS-number>"},
                       [](const std::vector<std::string> &args, CLIState &state) {
                         if (args.empty()) {
                           throw std::runtime_error("Router AS number required");
                         }

                         auto topology = state.getEngine().getTopology();
                         if (!topology) {
                           throw std::runtime_error("No topology loaded");
                         }

                         int as_number = std::stoi(args[0]);
                         auto router = topology->GetRouter(as_number);
                         if (!router) {
                           throw std::runtime_error("Router not found: AS" + args[0]);
                         }

                         return CommandResult(router->toString());
                       }});

  cli.registerCommand(
      "engine-status",
      {"engine-status",
       "Display current engine state and configuration",
       {"engine-status"},
       [](const std::vector<std::string> &args, CLIState &state) {
         auto &engine = Engine::Instance();
         std::stringstream ss;

         ss << "\nEngine Status\n";
         ss << "═════════════\n";
         ss << "State: " << engine.engineStateToString() << "\n";
         ss << "Last Error: " << (engine.getLastError().empty() ? "None" : engine.getLastError())
            << "\n";
         ss << "Last Info: " << (engine.getLastInfo().empty() ? "None" : engine.getLastInfo())
            << "\n";
         ss << "Topology Loaded: " << (engine.getTopology() ? "Yes" : "No") << "\n";

         if (engine.isExperimentRunning()) {
           const auto &expState = engine.getExperimentState();
           ss << "\nExperiment Status\n";
           ss << "─────────────────\n";
           ss << "Type: " << expState.type << "\n";
           ss << "Progress: " << expState.progress << "%\n";
           ss << "Status: " << expState.current_status << "\n";
         }

         return CommandResult(ss.str());
       }});

  cli.registerCommand("explore", {"explore",
                                  "Interactively explore routers and their relationships",
                                  {"explore", "explore <AS-number>"},
                                  [](const std::vector<std::string> &args, CLIState &state) {
                                    auto topology = state.getEngine().getTopology();
                                    if (!topology) {
                                      throw std::runtime_error("No topology loaded");
                                    }

                                    std::shared_ptr<Router> current_router;
                                    if (!args.empty()) {
                                      int as_number = std::stoi(args[0]);
                                      current_router = topology->GetRouter(as_number);
                                      if (!current_router) {
                                        throw std::runtime_error(
                                            "Router AS" + std::to_string(as_number) + " not found");
                                      }
                                    }

                                    return interactiveRouterExplorer(topology, current_router);
                                  }});
}

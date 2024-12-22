#include "cli/state.hpp"

CLIState::CLIState() : topology_(nullptr), current_experiment_(nullptr) {}
void CLIState::setTopology(std::shared_ptr<Topology> topology) { topology_ = topology; }
std::shared_ptr<Topology> CLIState::getTopology() const { return topology_; }
bool CLIState::hasTopology() const { return (topology_ != nullptr); }
ExperimentWorker *CLIState::getExperiment() const { return current_experiment_.get(); }
bool CLIState::hasExperiment() const { return (current_experiment_ != nullptr); }
void CLIState::addToHistory(const std::string &command) { command_history_.push_back(command); }
const std::vector<std::string> &CLIState::getHistory() const { return command_history_; }
void CLIState::clearHistory() { command_history_.clear(); }
Engine &CLIState::getEngine() { return engine_; }
void CLIState::setExperiment(std::unique_ptr<ExperimentWorker> experiment) {
  current_experiment_ = std::move(experiment);
}


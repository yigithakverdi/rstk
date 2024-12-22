#pragma once

#include "engine/engine.hpp"
#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include <memory>
#include <string>
#include <vector>

class CLIState {
public:
  CLIState();

  void setTopology(std::shared_ptr<Topology> topology);
  std::shared_ptr<Topology> getTopology() const;
  bool hasTopology() const;

  void setExperiment(std::unique_ptr<ExperimentWorker> experiment);
  ExperimentWorker *getExperiment() const;
  bool hasExperiment() const;

  void addToHistory(const std::string &command);
  const std::vector<std::string> &getHistory() const;
  void clearHistory();
  Engine &getEngine();


private:
  std::shared_ptr<Topology> topology_;
  std::unique_ptr<ExperimentWorker> current_experiment_;
  std::vector<std::string> command_history_;
  Engine &engine_{Engine::Instance()};
};

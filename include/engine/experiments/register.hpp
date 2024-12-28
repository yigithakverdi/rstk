#pragma once
#ifndef EXPERIMENT_REGISTRY_HPP
#define EXPERIMENT_REGISTRY_HPP

#include "engine/experiments/experiments.hpp"
#include "engine/experiments/registry/rhijack.hpp"
#include "engine/experiments/registry/rleak.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

class ExperimentWorker;

using ExperimentCreator = std::function<std::unique_ptr<ExperimentWorker>(
    std::queue<Trial> &, std::queue<double> &, std::shared_ptr<Topology>,
    const std::vector<std::string> &)>;

struct ExperimentInfo {
  std::string name;
  std::string description;
  std::vector<std::string> parameters;
  ExperimentCreator creator;
};

class ExperimentRegistry {
public:
  static ExperimentRegistry &Instance() {
    static ExperimentRegistry instance;
    return instance;
  }

  template <typename T>
  void registerExperiment(const std::string &name, const std::string &description,
                          const std::vector<std::string> &parameters) {
    ExperimentInfo info{name, description, parameters,
                        [](std::queue<Trial> &input, std::queue<double> &output,
                           std::shared_ptr<Topology> topology,
                           const std::vector<std::string> &args) {
                          return createExperiment<T>(input, output, topology, args);
                        }};
    experiments_[name] = info;
  }

  // Get list of available experiments
  std::vector<ExperimentInfo> listExperiments() const {
    std::vector<ExperimentInfo> result;
    for (const auto &[_, info] : experiments_) {
      result.push_back(info);
    }
    return result;
  }

  // Create an experiment instance
  std::unique_ptr<ExperimentWorker>
  createExperiment(const std::string &name, std::queue<Trial> &input, std::queue<double> &output,
                   std::shared_ptr<Topology> topology, const std::vector<std::string> &args) {

    auto it = experiments_.find(name);
    if (it == experiments_.end()) {
      throw std::runtime_error("Unknown experiment type: " + name);
    }

    return it->second.creator(input, output, topology, args);
  }

  // Check if an experiment type exists
  bool hasExperiment(const std::string &name) const {
    return experiments_.find(name) != experiments_.end();
  }

  // Get experiment info
  const ExperimentInfo &getExperimentInfo(const std::string &name) const {
    auto it = experiments_.find(name);
    if (it == experiments_.end()) {
      throw std::runtime_error("Unknown experiment type: " + name);
    }
    return it->second;
  }

private:
  ExperimentRegistry() = default;
  std::unordered_map<std::string, ExperimentInfo> experiments_;

  // Helper template to create specific experiment types
  template <typename T>
  static std::unique_ptr<ExperimentWorker>
  createExperiment(std::queue<Trial> &input, std::queue<double> &output,
                   std::shared_ptr<Topology> topology, const std::vector<std::string> &args) {

    if constexpr (std::is_same_v<T, RouteHijackExperiment>) { // Add this branch
      if (args.size() < 1) {
        throw std::runtime_error(
            "RouteHijackExperiment requires deployment strategy <selective|random>");
      }
      std::string deploymentType = args[0];
      return std::make_unique<RouteHijackExperiment>(input, output, topology, deploymentType);
    } else if (std::is_same_v<T, RouteLeakExperiment>) {
      if (args.size() < 1) {
        throw std::runtime_error(
          "RouteLeakExperiment requires deployment strategy <selective|random>");
      }
      std::string deploymentType = args[0];
      return std::make_unique<RouteLeakExperiment>(input, output, topology, deploymentType);
    }

    // Add more experiment types here
    // ...
    // ...

    else {
      throw std::runtime_error("Unknown experiment type");
    }
  }
};

// Macro to help register experiments
#define REGISTER_EXPERIMENT(TYPE, NAME, DESC, ...)                                                 \
  ExperimentRegistry::Instance().registerExperiment<TYPE>(NAME, DESC,                              \
                                                          std::vector<std::string>{__VA_ARGS__})

// Function to initialize all experiments
inline void initializeExperiments() {
  REGISTER_EXPERIMENT(RouteHijackExperiment, "route-hijack",
                      "Simulates ASPA deployment using CAIDA data",
                      "deployment_type: Type of deployment <random|selective>");
  REGISTER_EXPERIMENT(RouteLeakExperiment, "route-leak",
                      "Simulates route leak scenario on different percentages of ASPA",
                      "deployment_type: Type of deployment <random|selective");
}

#endif // EXPERIMENT_REGISTRY_HPP

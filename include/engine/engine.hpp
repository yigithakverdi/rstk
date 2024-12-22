#pragma once
#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "engine/experiments/experiments.hpp"
#include "engine/rpki/rpki.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/plugins.hpp"
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

// Forward declarations
class DeploymentStrategy;
class Protocol;
class ExperimentWorker;

// Plugin system types
using PluginID = std::string;
using PluginVersion = std::string;
using PluginMetadata = std::unordered_map<std::string, std::string>;

// Plugin interface
struct PluginInfo {
  PluginID id;
  PluginVersion version;
  std::string name;
  std::string description;
  PluginMetadata metadata;
};

// Plugin loading states
enum class PluginState { UNLOADED, LOADED, ERROR };

// Plugin configuration
struct PluginConfig {
  bool auto_load{true};
  std::string plugin_directory;
  std::vector<std::string> required_plugins;
  std::unordered_map<std::string, std::any> parameters;
};

// Plugin event types
enum class PluginEvent { LOADED, UNLOADED, ENABLED, DISABLED, ERROR };

// Plugin callback signature
using PluginEventCallback = std::function<void(PluginEvent, const PluginID &, const std::string &)>;

// Existing types from previous header...
enum class ExperimentEvent {
  STARTED,
  TRIAL_STARTED,
  TRIAL_COMPLETED,
  PAUSED,
  RESUMED,
  COMPLETED,
  ERROR
};

using EventCallback = std::function<void(ExperimentEvent, const std::string &)>;

struct EngineConfig {
  bool enable_rpki{false};
  bool enable_logging{false};
  std::string topology_file;
  std::string rpki_objects_file;
  int simulation_duration{0};
  PluginConfig plugin_config;
};

struct ExperimentState {
  std::string type;
  size_t total_trials{0};
  size_t completed_trials{0};
  double progress{0.0};
  std::string current_status;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
};

enum class EngineState { UNINITIALIZED, INITIALIZED, RUNNING, PAUSED, STOPPED, ERROR };

class Engine {
public:
  static Engine &Instance();
  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;

  // Plugin System Methods
  bool loadPlugin(const PluginID &id);
  bool unloadPlugin(const PluginID &id);
  bool enablePlugin(const PluginID &id);
  bool disablePlugin(const PluginID &id);
  bool configurePlugin(const PluginID &id, const PluginConfig &config);

  std::vector<PluginInfo> getLoadedPlugins() const;
  std::optional<PluginInfo> getPluginInfo(const PluginID &id) const;
  PluginState getPluginState(const PluginID &id) const;

  void registerPluginEventCallback(PluginEventCallback callback);
  void removePluginEventCallback(PluginEventCallback callback);

  // Initialization and configuration
  bool initialize(const EngineConfig &config);
  void shutdown();
  bool configure(const EngineConfig &config);

  // Experiment/Simulation Control
  bool startExperiment(const std::string &experiment_type,
                       const std::vector<std::string> &parameters);
  bool pauseExperiment();
  bool resumeExperiment();
  bool stopExperiment();
  bool isExperimentRunning() const;

  // Event handling
  void registerEventCallback(EventCallback callback);
  void removeEventCallback(EventCallback callback);

  // Monitoring and status
  ExperimentState getExperimentState() const;
  std::string getExperimentStatus() const;
  double getExperimentProgress() const;
  std::chrono::seconds getExperimentDuration() const;

  // Topology and component methods
  bool loadTopology(const std::string &filename);
  bool updateTopology(const std::shared_ptr<Topology> &topology);
  std::shared_ptr<Topology> getTopology() const;
  bool registerProtocol(const std::string &name, std::unique_ptr<Protocol> protocol);
  bool setDeploymentStrategy(std::unique_ptr<DeploymentStrategy> strategy);
  bool configureRPKI(const std::string &rpki_objects_file);

  const std::string &getLastError() const;
  const std::string &getLastInfo() const;

  EngineState getState() const;
  std::string engineStateToString() const;

  // Experiment management
  bool setUpExperiments();
  void updateExperimentProgress(size_t completed_trials);
  void cleanupExperiment();
  void setState(EngineState new_state);

private:
  Engine() = default;

  // Plugin management
  void initializePluginSystem();
  void loadRequiredPlugins();
  bool validatePluginDependencies(const PluginID &id);
  void notifyPluginEventListeners(PluginEvent event, const PluginID &id,
                                  const std::string &details);
  bool loadPluginDynamically(const std::string &plugin_path);
  void cleanupPlugins();

  // Event handling
  void notifyEventListeners(ExperimentEvent event, const std::string &details);

  // State tracking
  mutable std::string last_error_;
  mutable std::string last_info_;
  EngineState state_{EngineState::UNINITIALIZED};
  EngineConfig config_;

  // Plugin system state
  std::unordered_map<PluginID, std::unique_ptr<Protocol>> plugins_;
  std::unordered_map<PluginID, PluginState> plugin_states_;
  std::unordered_map<PluginID, PluginInfo> plugin_info_;
  std::vector<PluginEventCallback> plugin_callbacks_;

  // Experiment specific members
  std::unique_ptr<ExperimentWorker> current_experiment_;
  ExperimentState experiment_state_;
  std::vector<EventCallback> event_callbacks_;
  std::queue<Trial> input_queue_;
  std::queue<double> output_queue_;

  // Core components
  std::shared_ptr<Topology> topology_;
  std::shared_ptr<RPKI> rpki_;
  std::unique_ptr<DeploymentStrategy> deployment_strategy_;
  std::unordered_map<std::string, std::unique_ptr<Protocol>> registered_protocols_;
};

#endif // ENGINE_HPP

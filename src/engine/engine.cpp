#include "engine/engine.hpp"
#include "engine/experiments/register.hpp"

Engine &Engine::Instance() {
  static Engine instance;
  return instance;
}

// Add helper function to convert EngineState to string
std::string Engine::engineStateToString() const {
  switch (state_) {
  case EngineState::UNINITIALIZED:
    return "UNINITIALIZED";
  case EngineState::INITIALIZED:
    return "INITIALIZED";
  case EngineState::RUNNING:
    return "RUNNING";
  case EngineState::PAUSED:
    return "PAUSED";
  case EngineState::STOPPED:
    return "STOPPED";
  case EngineState::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

bool Engine::initialize(const EngineConfig &config) {
  if (state_ != EngineState::UNINITIALIZED) {
    last_error_ = "Engine already initialized";
    return false;
  }

  try {
    // Store configuration
    config_ = config;

    // Initialize RPKI if enabled
    if (config.enable_rpki) {
      rpki_ = std::make_shared<RPKI>();
      if (!config.rpki_objects_file.empty()) {
        if (!configureRPKI(config.rpki_objects_file)) {
          return false;
        }
      }
    }

    // Initialize plugin system
    try {
      initializePluginSystem();
    } catch (const std::exception &e) {
      last_error_ = "Plugin system initialization failed: " + std::string(e.what());
      return false;
    }

    // Load topology if specified
    if (!config.topology_file.empty()) {
      if (!loadTopology(config.topology_file)) {
        return false;
      }
    }

    setState(EngineState::INITIALIZED);
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Initialization failed: " + std::string(e.what());
    setState(EngineState::ERROR);
    return false;
  }
}

EngineState Engine::getState() const { return state_; }

void Engine::shutdown() {
  if (state_ == EngineState::UNINITIALIZED) {
    return;
  }

  // Stop any running experiment
  if (state_ == EngineState::RUNNING || state_ == EngineState::PAUSED) {
    stopExperiment();
  }

  // Clean up resources
  topology_.reset();
  rpki_.reset();
  deployment_strategy_.reset();
  registered_protocols_.clear();
  plugins_.clear();

  // Clear callbacks
  event_callbacks_.clear();
  plugin_callbacks_.clear();

  // Reset state trackers
  experiment_state_ = ExperimentState{};
  setState(EngineState::UNINITIALIZED);
}

bool Engine::configure(const EngineConfig &config) {
  if (state_ == EngineState::UNINITIALIZED) {
    last_error_ = "Engine not initialized";
    return false;
  }

  if (state_ == EngineState::RUNNING || state_ == EngineState::PAUSED) {
    last_error_ = "Cannot configure while experiment is running";
    return false;
  }

  try {
    // Update configuration
    config_ = config;

    // Update RPKI configuration if needed
    if (config.enable_rpki && !config.rpki_objects_file.empty()) {
      if (!configureRPKI(config.rpki_objects_file)) {
        return false;
      }
    }

    // Update topology if specified
    if (!config.topology_file.empty()) {
      if (!loadTopology(config.topology_file)) {
        return false;
      }
    }

    return true;

  } catch (const std::exception &e) {
    last_error_ = "Configuration failed: " + std::string(e.what());
    return false;
  }
}

bool Engine::startExperiment(const std::string &experiment_type,
                             const std::vector<std::string> &parameters) {
  if (state_ != EngineState::INITIALIZED) {
    last_error_ = "Engine not in initialized state";
    return false;
  }

  if (current_experiment_) {
    last_error_ = "An experiment is already running";
    return false;
  }

  try {
    // Create new experiment through registry
    auto &registry = ExperimentRegistry::Instance();
    if (!registry.hasExperiment(experiment_type)) {
      last_error_ = "Unknown experiment type: " + experiment_type;
      return false;
    }

    std::queue<Trial> input_queue;
    std::queue<double> output_queue;
    current_experiment_ = registry.createExperiment(experiment_type, input_queue, output_queue,
                                                    topology_, parameters);

    // Initialize experiment state
    experiment_state_.type = experiment_type;
    experiment_state_.start_time = std::chrono::steady_clock::now();
    experiment_state_.completed_trials = 0;
    experiment_state_.total_trials = current_experiment_->calculateTotalTrials();
    experiment_state_.progress = 0.0;
    experiment_state_.current_status = "Running";

    // Update engine state and notify listeners
    setState(EngineState::RUNNING);
    notifyEventListeners(ExperimentEvent::STARTED, "Started experiment: " + experiment_type);

    // Start experiment execution
    current_experiment_->run();
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Failed to start experiment: " + std::string(e.what());
    current_experiment_.reset();
    setState(EngineState::ERROR);
    return false;
  }
}

bool Engine::pauseExperiment() {
  if (state_ != EngineState::RUNNING) {
    last_error_ = "No running experiment to pause";
    return false;
  }

  try {
    if (current_experiment_) {
      current_experiment_->stop(); // Stop current execution
      setState(EngineState::PAUSED);
      experiment_state_.current_status = "Paused";
      notifyEventListeners(ExperimentEvent::PAUSED, "Experiment paused");
      return true;
    }
    return false;
  } catch (const std::exception &e) {
    last_error_ = "Failed to pause experiment: " + std::string(e.what());
    return false;
  }
}

bool Engine::resumeExperiment() {
  if (state_ != EngineState::PAUSED) {
    last_error_ = "No paused experiment to resume";
    return false;
  }

  try {
    if (current_experiment_) {
      setState(EngineState::RUNNING);
      experiment_state_.current_status = "Running";
      notifyEventListeners(ExperimentEvent::RESUMED, "Experiment resumed");
      current_experiment_->run(); // Resume execution
      return true;
    }
    return false;
  } catch (const std::exception &e) {
    last_error_ = "Failed to resume experiment: " + std::string(e.what());
    return false;
  }
}

bool Engine::stopExperiment() {
  if (state_ != EngineState::RUNNING && state_ != EngineState::PAUSED) {
    last_error_ = "No experiment to stop";
    return false;
  }

  try {
    if (current_experiment_) {
      current_experiment_->stop();
      experiment_state_.end_time = std::chrono::steady_clock::now();
      experiment_state_.current_status = "Stopped";

      // Clean up
      current_experiment_.reset();

      setState(EngineState::INITIALIZED);
      notifyEventListeners(ExperimentEvent::COMPLETED, "Experiment stopped");
      return true;
    }
    return false;
  } catch (const std::exception &e) {
    last_error_ = "Failed to stop experiment: " + std::string(e.what());
    return false;
  }
}

bool Engine::isExperimentRunning() const { return state_ == EngineState::RUNNING; }

void Engine::registerEventCallback(EventCallback callback) {
  if (callback) {
    // Add callback to the list if it's not null
    event_callbacks_.push_back(std::move(callback));
  }
}

void Engine::removeEventCallback(EventCallback callback) {
  // Compare callback targets to find and remove matching callbacks
  event_callbacks_.erase(std::remove_if(event_callbacks_.begin(), event_callbacks_.end(),
                                        [&callback](const EventCallback &cb) {
                                          return cb.target_type() == callback.target_type();
                                        }),
                         event_callbacks_.end());
}

ExperimentState Engine::getExperimentState() const { return experiment_state_; }

std::string Engine::getExperimentStatus() const {
  if (state_ == EngineState::UNINITIALIZED) {
    return "Not initialized";
  }
  if (!current_experiment_) {
    return "No experiment running";
  }
  return experiment_state_.current_status;
}

double Engine::getExperimentProgress() const {
  if (!current_experiment_) {
    return 0.0;
  }

  if (experiment_state_.total_trials == 0) {
    return 0.0;
  }

  return (static_cast<double>(experiment_state_.completed_trials) /
          static_cast<double>(experiment_state_.total_trials)) *
         100.0;
}

std::chrono::seconds Engine::getExperimentDuration() const {
  if (!current_experiment_) {
    return std::chrono::seconds(0);
  }

  auto end_time = (state_ == EngineState::RUNNING) ? std::chrono::steady_clock::now()
                                                   : experiment_state_.end_time;

  return std::chrono::duration_cast<std::chrono::seconds>(end_time - experiment_state_.start_time);
}

bool Engine::loadTopology(const std::string &filename) {
  try {
    // Create parser and RPKI instance
    Parser parser;
    auto rpki = std::make_shared<RPKI>();

    // Parse relationships from file
    auto relations = parser.GetAsRelationships(filename);

    // Create new topology
    topology_ = std::make_shared<Topology>(relations, rpki);

    // Optionally store success info (if you want to retrieve it later)
    last_info_ = "Loaded topology from " + filename + "\nTopology has " +
                 std::to_string(relations.size()) + " relationships";

    return true;
  } catch (const std::exception &e) {
    last_error_ = "Failed to load topology: " + std::string(e.what());
    return false;
  }
}

bool Engine::updateTopology(const std::shared_ptr<Topology> &topology) {
  if (!topology) {
    last_error_ = "Cannot update with null topology";
    return false;
  }

  if (state_ == EngineState::RUNNING || state_ == EngineState::PAUSED) {
    last_error_ = "Cannot update topology while experiment is running";
    return false;
  }

  try {
    topology_ = topology;
    return true;
  } catch (const std::exception &e) {
    last_error_ = "Failed to update topology: " + std::string(e.what());
    return false;
  }
}

std::shared_ptr<Topology> Engine::getTopology() const { return topology_; }

bool Engine::registerProtocol(const std::string &name, std::unique_ptr<Protocol> protocol) {
  if (!protocol) {
    last_error_ = "Cannot register null protocol";
    return false;
  }

  try {
    if (registered_protocols_.find(name) != registered_protocols_.end()) {
      last_error_ = "Protocol already registered with name: " + name;
      return false;
    }

    registered_protocols_[name] = std::move(protocol);
    return true;
  } catch (const std::exception &e) {
    last_error_ = "Failed to register protocol: " + std::string(e.what());
    return false;
  }
}

bool Engine::setDeploymentStrategy(std::unique_ptr<DeploymentStrategy> strategy) {
  if (!strategy) {
    last_error_ = "Cannot set null deployment strategy";
    return false;
  }

  if (state_ == EngineState::RUNNING || state_ == EngineState::PAUSED) {
    last_error_ = "Cannot change deployment strategy while experiment is running";
    return false;
  }

  try {
    deployment_strategy_ = std::move(strategy);
    if (topology_) {
      topology_->setDeploymentStrategy(std::move(strategy));
    }
    return true;
  } catch (const std::exception &e) {
    last_error_ = "Failed to set deployment strategy: " + std::string(e.what());
    return false;
  }
}

bool Engine::configureRPKI(const std::string &rpki_objects_file) {
  if (!rpki_) {
    rpki_ = std::make_shared<RPKI>();
  }

  try {
    if (!rpki_->loadUSPAsFromFile(rpki_objects_file)) {
      last_error_ = "Failed to load RPKI objects from file";
      return false;
    }

    // If topology exists, update its RPKI instance
    if (topology_) {
      topology_->RPKIInstance = rpki_;
    }

    return true;
  } catch (const std::exception &e) {
    last_error_ = "RPKI configuration failed: " + std::string(e.what());
    return false;
  }
}

void Engine::initializePluginSystem() {
  // Clear any existing plugin data
  plugins_.clear();
  plugin_states_.clear();
  plugin_info_.clear();
  plugin_callbacks_.clear();

  try {
    // If auto-load is enabled, load required plugins
    if (config_.plugin_config.auto_load) {
      loadRequiredPlugins();
    }
  } catch (const std::exception &e) {
    throw std::runtime_error("Plugin system initialization failed: " + std::string(e.what()));
  }
}

void Engine::loadRequiredPlugins() {
  for (const auto &plugin_id : config_.plugin_config.required_plugins) {
    std::string plugin_path = config_.plugin_config.plugin_directory + "/" + plugin_id;
    if (!loadPluginDynamically(plugin_path)) {
      throw std::runtime_error("Failed to load required plugin: " + plugin_id);
    }
  }
}

bool Engine::validatePluginDependencies(const PluginID &id) {
  auto it = plugin_info_.find(id);
  if (it == plugin_info_.end()) {
    last_error_ = "Plugin not found: " + id;
    return false;
  }

  // Check if all required plugins are loaded
  const auto &metadata = it->second.metadata;
  auto deps_it = metadata.find("dependencies");
  if (deps_it != metadata.end()) {
    std::istringstream iss(deps_it->second);
    std::string dep;
    while (std::getline(iss, dep, ',')) {
      if (plugin_states_.find(dep) == plugin_states_.end() ||
          plugin_states_[dep] != PluginState::LOADED) {
        last_error_ = "Missing dependency: " + dep + " for plugin: " + id;
        return false;
      }
    }
  }
  return true;
}

void Engine::notifyPluginEventListeners(PluginEvent event, const PluginID &id,
                                        const std::string &details) {
  for (const auto &callback : plugin_callbacks_) {
    try {
      callback(event, id, details);
    } catch (const std::exception &e) {
      // Log error but continue notifying other listeners
      last_error_ = "Plugin event listener error: " + std::string(e.what());
    }
  }
}

bool Engine::loadPluginDynamically(const std::string &plugin_path) {
  try {
    // For now, we'll implement a basic plugin loading mechanism
    // This should be enhanced with actual dynamic loading (e.g., dlopen) in the future

    // Extract plugin ID from path
    std::string plugin_id = plugin_path.substr(plugin_path.find_last_of("/") + 1);

    // Create dummy plugin info for now
    PluginInfo info{.id = plugin_id,
                    .version = "1.0",
                    .name = plugin_id,
                    .description = "Dynamically loaded plugin",
                    .metadata = {}};

    plugin_info_[plugin_id] = info;
    plugin_states_[plugin_id] = PluginState::LOADED;

    notifyPluginEventListeners(PluginEvent::LOADED, plugin_id, "Plugin loaded successfully");
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Failed to load plugin: " + std::string(e.what());
    return false;
  }
}

void Engine::cleanupPlugins() {
  for (const auto &[id, _] : plugins_) {
    try {
      notifyPluginEventListeners(PluginEvent::UNLOADED, id, "Plugin unloaded during cleanup");
    } catch (...) {
      // Ignore errors during cleanup
    }
  }

  plugins_.clear();
  plugin_states_.clear();
  plugin_info_.clear();
  plugin_callbacks_.clear();
}

void Engine::setState(EngineState new_state) {
  if (state_ != new_state) {
    state_ = new_state;
    if (state_ == EngineState::ERROR) {
      cleanupPlugins(); // Cleanup plugins on error state
    }
  }
}

void Engine::notifyEventListeners(ExperimentEvent event, const std::string &details) {
  for (const auto &callback : event_callbacks_) {
    try {
      callback(event, details);
    } catch (const std::exception &e) {
      // Log error but continue notifying other listeners
      // We don't want one failed callback to prevent others from being notified
      last_error_ = "Event listener error: " + std::string(e.what());
    } catch (...) {
      // Catch any other unexpected errors
      last_error_ = "Unknown error in event listener";
    }
  }
}

bool Engine::setUpExperiments() {
  try {
    initializeExperiments(); // This calls the global function that registers experiments
    return true;
  } catch (const std::exception &e) {
    last_error_ = "Failed to initialize experiments: " + std::string(e.what());
    return false;
  }
}

void Engine::updateExperimentProgress(size_t completed_trials) {
  if (!current_experiment_) {
    return;
  }

  experiment_state_.completed_trials = completed_trials;
  experiment_state_.total_trials = current_experiment_->calculateTotalTrials();

  if (experiment_state_.total_trials > 0) {
    experiment_state_.progress = (static_cast<double>(completed_trials) /
                                  static_cast<double>(experiment_state_.total_trials)) *
                                 100.0;
  }

  // Notify listeners about progress
  notifyEventListeners(ExperimentEvent::TRIAL_COMPLETED,
                       "Completed trial " + std::to_string(completed_trials) + " of " +
                           std::to_string(experiment_state_.total_trials));

  // Check if experiment is complete
  if (completed_trials >= experiment_state_.total_trials) {
    experiment_state_.current_status = "Completed";
    experiment_state_.end_time = std::chrono::steady_clock::now();
    current_experiment_.reset();        // Clear the experiment
    setState(EngineState::INITIALIZED); // Reset state
    notifyEventListeners(ExperimentEvent::COMPLETED, "Experiment completed successfully");
  }
}

void Engine::cleanupExperiment() {
  if (current_experiment_) {
    // Stop the experiment if it's running
    current_experiment_->stop();

    // Clear experiment state
    experiment_state_ = ExperimentState{};

    // Reset experiment pointer
    current_experiment_.reset();
  }
}

const std::string &Engine::getLastError() const { return last_error_; }
const std::string &Engine::getLastInfo() const { return last_info_; }

#pragma once

#include "engine/ebus.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

class IModule {
public:
  virtual ~IModule() = default;
  virtual void initialize(std::shared_ptr<ebus> bus) = 0;
  virtual void cleanup() = 0;
  virtual std::string name() const = 0;
  virtual bool isReady() const = 0;

  bool getInitialized() const { return initialized_; }
  bool getIsStage() const { return isStage; }

private:
  bool isStage = false;
  bool initialized_{false};
  std::shared_ptr<ebus> bus;
};

class pipeline {
public:
  pipeline(std::shared_ptr<ebus> bus);
  void addModule(std::shared_ptr<IModule> mod);
  void start();
  void stop();
  void handleModuleComplete(const std::string &moduleName);
  void checkAllModulesComplete();
  std::future<void> waitForCompletion();

  std::vector<std::shared_ptr<IModule>> getStages() const { return stages_; }

private:
  std::shared_ptr<ebus> bus_;
  std::vector<std::shared_ptr<IModule>> stages_;
  std::unordered_set<std::string> completedStages_;
  std::mutex completionMutex_;
  std::condition_variable completionCV_;
  std::promise<void> completionPromise_;
  bool isComplete_{false};
};

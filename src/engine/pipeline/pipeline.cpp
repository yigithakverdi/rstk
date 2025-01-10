#include "engine/pipeline/pipeline.hpp"
#include "engine/ebus.hpp"
#include "engine/events/complete.hpp"

pipeline::pipeline(std::shared_ptr<ebus> bus) : bus_(bus) {
  bus_->subscribe<ModuleCompleteEvent>(
      [this](std::shared_ptr<IEvent> evt) {
        auto moduleEvt = std::static_pointer_cast<ModuleCompleteEvent>(evt);
        auto moduleName = moduleEvt->getData<std::string>("module_name");
        handleModuleComplete(moduleName);
      },
      eventPriority::HIGH);
}
void pipeline::addModule(std::shared_ptr<IModule> mod) { stages_.push_back(mod); }

void pipeline::start() {
  for (auto &stage : stages_) {
    stage->initialize(bus_);
  }
}

void pipeline::stop() {
  std::unique_lock<std::mutex> lock(completionMutex_);
  completionCV_.wait(lock, [this]() { return isComplete_; });

  for (auto &module : stages_) {
    module->cleanup();
  }
}
void pipeline::handleModuleComplete(const std::string &moduleName) {
  std::lock_guard<std::mutex> lock(completionMutex_);
  completedStages_.insert(moduleName);
  checkAllModulesComplete();
}

std::future<void> pipeline::waitForCompletion() { return completionPromise_.get_future(); }
void pipeline::checkAllModulesComplete() {
  if (!isComplete_ && completedStages_.size() == stages_.size()) {
    isComplete_ = true;
    auto completeEvt = std::make_shared<AllModulesCompleteEvent>();
    bus_->publish(completeEvt);
    completionPromise_.set_value();
    completionCV_.notify_all();
  }
}

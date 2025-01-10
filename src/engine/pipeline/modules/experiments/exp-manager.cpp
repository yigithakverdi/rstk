#include "engine/pipeline/modules/experiments/exp-manager.hpp"

void ExperimentModule::initialize(std::shared_ptr<ebus> bus) {
  bus_ = bus;
  initialized_ = true;

  bus_->subscribe<TopologyReadyEvent>([this](std::shared_ptr<IEvent> evt) { handleTopologyReady(evt); });
  bus_->subscribe<ExperimentStartEvent>([this](std::shared_ptr<IEvent> evt) { handleExperimentStart(evt); });
}

void ExperimentModule::cleanup() {
  initialized_ = false;
  topology_.reset();
  currexp_.reset();
}

std::string ExperimentModule::name() const { return "ExperimentModule"; }
bool ExperimentModule::isReady() const { return initialized_; }
void ExperimentModule::handleTopologyReady(std::shared_ptr<IEvent> evt) {
  auto topology_evt = std::static_pointer_cast<TopologyReadyEvent>(evt);
  topology_ = topology_evt->getData<std::shared_ptr<topology>>("topology");
}

void ExperimentModule::handleExperimentStart(std::shared_ptr<IEvent> evt) {
  auto start_evt = std::static_pointer_cast<ExperimentStartEvent>(evt);

  std::string exp_type = start_evt->getData<std::string>("type");
  auto params = start_evt->getData<std::vector<std::string>>("parameters");

  std::queue<trial> input_queue;
  std::queue<double> output_queue;
  currexp_ = expregister::instance().create(exp_type, input_queue, output_queue, topology_, params);
  currexp_->run();
}

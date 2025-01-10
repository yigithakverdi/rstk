#include "engine/pipeline/modules/topology/topo-loader.hpp"
#include "engine/events/complete.hpp"
#include "engine/events/startup.hpp"
#include "engine/events/topology/etopology.hpp"
#include <iostream>

void TopologyModule::initialize(std::shared_ptr<ebus> bus) {
  bus_ = bus;
  bus_->subscribe<StartupEvent>(
      [this](std::shared_ptr<IEvent> evt) {
        auto startupEvt = std::static_pointer_cast<StartupEvent>(evt);
        std::string topoConfig = startupEvt->getData<std::string>("topology_config");
        topologyConfig_ = topoConfig;
        handleLoadTopology(evt);
      },
      eventPriority::HIGH);

  initialized_ = true;
}

void TopologyModule::cleanup() { currentTopology_.reset(); }
void TopologyModule::handleLoadTopology(std::shared_ptr<IEvent> evt) {
  try {
    currentTopology_ = std::make_shared<topology>();
    if (!validateTopology(currentTopology_)) {
      throw std::runtime_error("Topology validation failed");
    }

    handleTopologyBuilt(currentTopology_);

  } catch (const std::exception &e) {
    auto errorEvt = std::make_shared<BaseEvent>();
    errorEvt->setData("error", std::string("Topology loading failed: ") + e.what());
    bus_->publish(errorEvt, true);
  }
}

void TopologyModule::handleTopologyBuilt(std::shared_ptr<topology> topo) {
  auto readyEvt = std::make_shared<TopologyReadyEvent>(topo);
  bus_->publish(readyEvt).get();

  auto completeEvt = std::make_shared<ModuleCompleteEvent>(name());
  bus_->publish(completeEvt);

  std::cout << "Topology loaded and ready for experiments" << std::endl;
  std::cout << "Topology loaded with " << topo->getGraph()->getNodes().size() << " routers" << std::endl;
}

bool TopologyModule::validateTopology(std::shared_ptr<topology> topo) {
  if (!topo)
    return false;

  const auto &graph = topo->getGraph();
  if (!graph || graph->getNodes().empty()) {
    return false;
  }

  if (!topo->getRPKI()) {
    return false;
  }

  auto t1 = topo->getTierOne();
  auto t2 = topo->getTierTwo();
  auto t3 = topo->getTierThree();

  if (t1.empty() && t2.empty() && t3.empty()) {
    return false;
  }

  return true;
}

std::string TopologyModule::name() const { return "TopologyModule"; }
bool TopologyModule::isReady() const { return initialized_ && currentTopology_ != nullptr; }

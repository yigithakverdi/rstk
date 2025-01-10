#include "engine/core.hpp"
#include "engine/ebus.hpp"
/*#include "engine/pipeline/modules/experiments/exp-manager.hpp"*/
#include "engine/pipeline/modules/topology/topo-loader.hpp"
#include "engine/pipeline/pipeline.hpp"

engine::~engine() {}
engine::engine() {
  bus_ = std::make_shared<ebus>();
  pipe_ = std::make_shared<pipeline>(bus_);

  pipe_->addModule(std::make_shared<TopologyModule>());
  /*pipe_->addModule(std::make_shared<ExperimentModule>());*/
}



#include "engine/core.hpp"
#include "engine/events/startup.hpp"
#include "engine/pipeline/modules/topology/topo-loader.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  engine e;
  auto bus = e.getBus();
  auto pipe = e.getPipeline();

  if (!bus || !pipe) {
    return 1;
  }

  bus->start();
  pipe->start();

  try {
    auto startupEvt = std::make_shared<StartupEvent>("default");
    bus->publish(startupEvt);

    // Wait for all modules to complete
    pipe->waitForCompletion().get();

    // Now safe to stop
    pipe->stop();
    bus->stop();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
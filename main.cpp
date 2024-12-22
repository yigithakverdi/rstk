#include "cli/cli.hpp"
#include "engine/engine.hpp"
#include "engine/experiments/register.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  try {
    // Initialize Engine with default configuration
    EngineConfig config;
    config.enable_rpki = true;
    config.enable_logging = true;

    auto &engine = Engine::Instance();
    if (!engine.initialize(config)) {
      throw std::runtime_error(engine.getLastError());
    }

    // Initialize experiments through Engine instead of directly
    if (!engine.setUpExperiments()) {
      throw std::runtime_error(engine.getLastError());
    }

    // Create and run CLI instance
    CLI cli;
    cli.run();

    // Cleanup
    engine.shutdown();
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}

#include "engine/experiments/experiments.hpp"
#include "engine/engine.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <chrono>
#include <memory>
#include <thread>

ExperimentProgress::ExperimentProgress(size_t total)
    : total_(total), start_time_(std::chrono::steady_clock::now()) {}

std::string ExperimentProgress::getBar() {
  const int width = 30;
  float progress = static_cast<float>(current_) / total_;
  int pos = static_cast<int>(width * progress);

  std::string bar;
  bar.reserve(width);
  for (int i = 0; i < width; ++i) {
    if (i < pos)
      bar += "█";
    else if (i == pos)
      bar += "▓";
    else
      bar += "░";
  }
  return bar;
}

void ExperimentProgress::update(size_t current) { current_ = current; }

std::string ExperimentProgress::estimateTimeRemaining() {
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

  if (current_ == 0)
    return "calculating...";

  double rate = static_cast<double>(current_) / elapsed;
  int remaining_seconds = static_cast<int>((total_ - current_) / rate);

  return formatDuration(remaining_seconds);
}

ExperimentWorker::ExperimentWorker(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                                   std::shared_ptr<Topology> topology)
    : topology_(topology), input_queue_(input_queue), output_queue_(output_queue), stopped_(false) {
}

void ExperimentWorker::stop() { stopped_ = true; }

void ExperimentWorker::run() {
  std::cout << "Initializing trials..." << std::endl;
  initializeTrial();

  size_t totalTrials = input_queue_.size();
  ExperimentProgress progress(totalTrials);

  int trialCount = 0;
  while (!stopped_ && !input_queue_.empty()) {
    Trial trial = input_queue_.front();
    input_queue_.pop();

    progress.update(++trialCount);
    output_queue_.push(runTrial(trial));

    std::cout << "\r" << std::string(80, ' ') << "\r";
    std::cout << "\rProgress: [" << progress.getBar() << "] " << trialCount << "/" << totalTrials
              << " (ETA: " << progress.estimateTimeRemaining() << ")" << std::flush;
  }
  std::cout << std::endl;

  // Add this to notify completion
  auto &engine = Engine::Instance();
  engine.updateExperimentProgress(totalTrials);
  engine.setState(EngineState::INITIALIZED); // Reset state after completion
}

double ExperimentWorker::calculateAttackerSuccess(std::shared_ptr<Router> attacker,
                                                  std::shared_ptr<Router> victim) {
  return 0.0;
}

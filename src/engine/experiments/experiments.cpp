#include "engine/experiments/experiments.hpp"
#include "engine/engine.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <chrono>
#include <memory>

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

// In ExperimentWorker::run()
void ExperimentWorker::run() {
  std::cout << "Run not set, you need to overload it under specific experiment class" << std::endl;
}

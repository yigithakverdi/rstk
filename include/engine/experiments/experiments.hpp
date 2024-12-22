#pragma once
#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include <chrono>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

class Topology;
class Router;

struct Trial {
  std::shared_ptr<Router> victim;
  std::shared_ptr<Router> attacker;
};

class ExperimentWorker {
public:
  ExperimentWorker(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                   std::shared_ptr<Topology> topology);
  virtual ~ExperimentWorker() = default;

  void stop();
  void run();

  double calculateAttackerSuccess(std::shared_ptr<Router> attacker, std::shared_ptr<Router> victim);
  virtual size_t calculateTotalTrials() const = 0;
  std::queue<Trial> &input_queue_;
  std::queue<double> &output_queue_;

protected:
  virtual double runTrial(const Trial &trial) = 0;
  virtual void initializeTrial() = 0;
  virtual bool setupTopology() { return (topology_ != nullptr); }

  std::shared_ptr<Topology> topology_;
  bool stopped_;

  // Worker for parallel trail execution
  static void threadWorker(std::shared_ptr<ExperimentWorker> worker);
};

class ExperimentProgress {
public:
  explicit ExperimentProgress(size_t total);

  void update(size_t current);
  std::string getBar();
  std::string estimateTimeRemaining();

private:
  size_t total_;
  size_t current_{0};
  std::chrono::steady_clock::time_point start_time_;
  std::string formatDuration(int seconds) {
    int hours = seconds / 3600;
    seconds %= 3600;
    int minutes = seconds / 60;
    seconds %= 60;

    std::stringstream ss;
    if (hours > 0)
      ss << hours << "h ";
    if (minutes > 0)
      ss << minutes << "m ";
    ss << seconds << "s";
    return ss.str();
  }
};

#endif // EXPERIMENTS_HPP

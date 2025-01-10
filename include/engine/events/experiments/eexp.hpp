#pragma once
#ifndef EEXP_HPP
#define EEXP_HPP

#include "engine/ebus.hpp"

// Event to start a specific experiment type
class ExperimentStartEvent : public BaseEvent {
public:
  ExperimentStartEvent(const std::string &type, const std::vector<std::string> &params) {
    setData("type", type);
    setData("parameters", params);
  }

  std::type_index getType() const override { return std::type_index(typeid(ExperimentStartEvent)); }
  std::string getName() const override { return "ExperimentStart"; }
};

// Event when a trial completes
class TrialCompletedEvent : public BaseEvent {
public:
  TrialCompletedEvent(size_t trial_number, double result) {
    setData("trial_number", trial_number);
    setData("result", result);
  }

  std::type_index getType() const override { return std::type_index(typeid(TrialCompletedEvent)); }
  std::string getName() const override { return "TrialCompleted"; }
};

#endif // EEXP_HPP

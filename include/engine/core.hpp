#pragma once
#ifndef CORE_HPP
#define CORE_HPP

#include "engine/ebus.hpp"
#include "engine/pipeline/pipeline.hpp"

class ebus;
class pipeline;
class engine {
public:
  engine();
  ~engine();

  std::shared_ptr<ebus> getBus() { return bus_; }
  std::shared_ptr<pipeline> getPipeline() { return pipe_; }

private:
  std::shared_ptr<ebus> bus_;
  std::shared_ptr<pipeline> pipe_;
};

#endif // CORE_HPP

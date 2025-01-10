#pragma once
#ifndef STARTUP_HPP
#define STARTUP_HPP

#include "engine/ebus.hpp"

class StartupEvent : public BaseEvent {
public:
  explicit StartupEvent(const std::string &topoConfig) { setData("topology_config", topoConfig); }
  std::type_index getType() const override { return std::type_index(typeid(StartupEvent)); }
  std::string getName() const override { return "StartupEvent"; }

private:
};

#endif // ETOP_HPP

// manager.cpp
#include "proto/manager.hpp"
#include "proto/policy.hpp"
#include "proto/proto.hpp"

protofactory &protofactory::instance() {
  static protofactory instance;
  return instance;
}

IProto *protofactory::create(const std::string &name) const {
  if (name == "base") {
    return new IProto(std::make_unique<IPolicy>());
  }
  return nullptr;
}

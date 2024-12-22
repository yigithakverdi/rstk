#pragma once

// ProtocolFactory.hpp
#ifndef PROTOCOL_FACTORY_HPP
#define PROTOCOL_FACTORY_HPP

#include "plugins/base/base.hpp"
#include <memory>

class ProtocolFactory {
public:
  static ProtocolFactory &Instance() {
    static ProtocolFactory instance;
    return instance;
  }

  std::unique_ptr<Protocol> CreateProtocol(int ASNumber);
  void UpdateSpecifications();

private:
  ProtocolFactory() = default;
  ProtocolFactory(const ProtocolFactory &) = delete;
  ProtocolFactory &operator=(const ProtocolFactory &) = delete;
};

#endif // PROTOCOL_FACTORY_HPP

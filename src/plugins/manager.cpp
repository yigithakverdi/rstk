#include "plugins/manager.hpp"

std::unique_ptr<Protocol> ProtocolFactory::CreateProtocol(int ASNumber) {
  if (ASNumber % 2 == 0) {
    return std::make_unique<BaseProtocol>();
  } else {
    return std::make_unique<BaseProtocol>();
  }
}
void ProtocolFactory::UpdateSpecifications() {}

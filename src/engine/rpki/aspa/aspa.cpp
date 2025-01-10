#include "engine/rpki/aspa/aspa.hpp"

aspaobj::aspaobj() {}
aspaobj::~aspaobj() {}
std::string aspaobj::getCustomer() const { return customer; }
const std::vector<std::string> &aspaobj::getProviders() const { return providers; }
const std::vector<unsigned char> &aspaobj::getSignature() const { return signature; }
void aspaobj::setCustomer(const std::string &customer) { this->customer = customer; }
void aspaobj::addProvider(const std::string &provider) { providers.push_back(provider); }
void aspaobj::setSignature(const std::vector<unsigned char> &signature) { this->signature = signature; }
std::string aspaobj::getCustomer() { return customer; }
const std::vector<std::string> &aspaobj::getProviders() { return providers; }
const std::vector<unsigned char> &aspaobj::getSignature() { return signature; }
aspaobj::aspaobj(const std::string &customer, const std::set<std::string> &providers) {
  this->customer = customer;
  for (const auto &provider : providers) {
    this->providers.push_back(provider);
  }
}

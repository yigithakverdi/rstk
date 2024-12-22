#include "engine/rpki/rpki.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>


int ASPAObject::getCustomerAS() const { return customerAS; }

const std::vector<int> &ASPAObject::getProviderASes() const { return providerASes; }

const std::vector<unsigned char> &ASPAObject::getSignature() const { return signature; }

void ASPAObject::setCustomerAS(int customerAS_) { customerAS = customerAS_; }

void ASPAObject::setProviderASes(const std::vector<int> &providerASes_) { providerASes = providerASes_; }

void ASPAObject::setSignature(const std::vector<unsigned char> &signature_) { signature = signature_; }

void RPKI::addROA(int asNumber, int prefix) { ROAs[asNumber].insert(prefix); }

void RPKI::removeROA(int asNumber, int prefix) {
  auto it = ROAs.find(asNumber);
  if (it != ROAs.end()) {
    it->second.erase(prefix);
    if (it->second.empty()) {
      ROAs.erase(it);
    }
  }
}

const std::map<int, std::set<int>> &RPKI::getROAs() const { return ROAs; }

void RPKI::addUSPA(const ASPAObject &uspa) { USPAS[uspa.getCustomerAS()] = uspa; }

void RPKI::removeUSPA(int customerAS) { USPAS.erase(customerAS); }

const std::map<int, ASPAObject> &RPKI::getUSPAs() const { return USPAS; }

bool RPKI::validateRoute(int originAS, int prefix) const {
  auto it = ROAs.find(originAS);
  if (it != ROAs.end()) {
    return it->second.find(prefix) != it->second.end();
  }
  return false;
}

bool RPKI::validateASPA(int customerAS, int providerAS) const {
  auto it = USPAS.find(customerAS);
  if (it != USPAS.end()) {
    const ASPAObject &uspa = it->second;
    const std::vector<int> &providers = uspa.getProviderASes();
    return std::find(providers.begin(), providers.end(), providerAS) != providers.end();
  }
  return false;
}

bool RPKI::loadROAsFromFile(const std::string &filename) {
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    return false;
  }

  clearROAs();

  std::string line;
  while (std::getline(infile, line)) {
    if (line.empty() || line[0] == '#')
      continue;

    int asNumber, prefix;
    if (parseROALine(line, asNumber, prefix)) {
      addROA(asNumber, prefix);
    }
  }

  infile.close();
  return true;
}

bool RPKI::loadUSPAsFromFile(const std::string &filename) {
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    return false;
  }

  clearUSPAs();

  std::string line;
  while (std::getline(infile, line)) {
    if (line.empty() || line[0] == '#')
      continue;

    ASPAObject uspa;
    if (parseUSPALine(line, uspa)) {
      addUSPA(uspa);
    }
  }

  infile.close();
  return true;
}

void RPKI::clearROAs() { ROAs.clear(); }

void RPKI::clearUSPAs() { USPAS.clear(); }

bool RPKI::parseROALine(const std::string &line, int &asNumber, int &prefix) {
  std::istringstream iss(line);
  std::string token;

  if (!(iss >> token))
    return false;

  try {
    asNumber = std::stoi(token);
  } catch (...) {
    return false;
  }

  if (!(iss >> token))
    return false;

  try {
    prefix = std::stoi(token);
  } catch (...) {
    return false;
  }

  return true;
}

bool RPKI::parseUSPALine(const std::string &line, ASPAObject &uspa) {
  std::istringstream iss(line);
  std::string token;

  if (!(iss >> token))
    return false;
  int customerAS;
  try {
    customerAS = std::stoi(token);
  } catch (...) {
    return false;
  }

  if (!(iss >> token))
    return false;
  std::vector<int> providerASes;
  std::istringstream providersStream(token);
  std::string provider;
  while (std::getline(providersStream, provider, ',')) {
    try {
      providerASes.push_back(std::stoi(provider));
    } catch (...) {
    }
  }

  if (!(iss >> token))
    return false;
  std::vector<unsigned char> signature;
  if (token.size() % 2 != 0)
    return false;

  for (size_t i = 0; i < token.size(); i += 2) {
    std::string byteString = token.substr(i, 2);
    unsigned char byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));
    signature.push_back(byte);
  }

  uspa = ASPAObject(customerAS, providerASes, signature);
  return true;
}

#include "engine/rpki/rpki.hpp"

rpki::rpki() {}

rpki::~rpki() {
  clearROAs();
  clearUSPAs();
}

void rpki::addROA(const std::string &asn, const std::set<std::string> &prefixes) { roas_[asn] = prefixes; }
void rpki::addUSPA(const std::string &asn, const std::set<std::string> &prefixes) { uspas_[asn] = prefixes; }
void rpki::removeROA(const std::string &asn) { roas_.erase(asn); }
void rpki::removeUSPA(const std::string &asn) { uspas_.erase(asn); }
void rpki::clearROAs() { roas_.clear(); }
void rpki::clearUSPAs() { uspas_.clear(); }

std::vector<roa> rpki::getROAs() const {
  std::vector<roa> result;
  for (const auto &[asn, prefixes] : roas_) {
    result.emplace_back(asn, prefixes);
  }
  return result;
}

std::vector<aspaobj> rpki::getUSPAs() const {
  std::vector<aspaobj> result;
  for (const auto &[asn, prefixes] : uspas_) {
    result.emplace_back(asn, prefixes);
  }
  return result;
}

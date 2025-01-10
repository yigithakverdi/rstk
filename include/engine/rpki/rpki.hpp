#pragma once
#ifndef RPKI_HPP
#define RPKI_HPP

#include "engine/rpki/aspa/aspa.hpp"
#include "engine/rpki/roa/roa.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

class rpki {
public:
  rpki();
  ~rpki();

  void addROA(const std::string &asn, const std::set<std::string> &prefixes);
  void addUSPA(const std::string &asn, const std::set<std::string> &prefixes);
  void removeROA(const std::string &asn);
  void removeUSPA(const std::string &asn);

  std::vector<roa> getROAs() const;
  std::vector<aspaobj> getUSPAs() const;

  void clearROAs();
  void clearUSPAs();

private:
  std::map<std::string, std::set<std::string>> roas_;
  std::map<std::string, std::set<std::string>> uspas_;
};

#endif // RPKI_HPP

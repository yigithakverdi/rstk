#pragma once

#ifndef RPKI_HPP
#define RPKI_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

class ASPAObject {
public:
  ASPAObject() = default;
  ASPAObject(int customerAS, const std::vector<int> &providerASes, const std::vector<unsigned char> &signature);

  // Getters
  int getCustomerAS() const;
  const std::vector<int> &getProviderASes() const;
  const std::vector<unsigned char> &getSignature() const;

  // Setters
  void setCustomerAS(int customerAS);
  void setProviderASes(const std::vector<int> &providerASes);
  void setSignature(const std::vector<unsigned char> &signature);
  int customerAS;
  std::vector<int> providerASes;
  std::vector<unsigned char> signature;

private:
};

class RPKI {
public:
  RPKI() = default;

  // ROA Management
  void addROA(int asNumber, int prefix);
  void removeROA(int asNumber, int prefix);
  const std::map<int, std::set<int>> &getROAs() const;

  // USPA Management
  void addUSPA(const ASPAObject &uspa);
  void removeUSPA(int customerAS);
  const std::map<int, ASPAObject> &getUSPAs() const;

  // Validation
  bool validateRoute(int originAS, int prefix) const;
  bool validateASPA(int customerAS, int providerAS) const;

  // Loading Data
  bool loadROAsFromFile(const std::string &filename);
  bool loadUSPAsFromFile(const std::string &filename);

  // Utility
  void clearROAs();
  void clearUSPAs();

  std::map<int, std::set<int>> ROAs; // AS Number -> Set of Prefixes
  std::map<int, ASPAObject> USPAS;   // Customer AS -> ASPAObject

private:
  // Helper Functions
  bool parseROALine(const std::string &line, int &asNumber, int &prefix);
  bool parseUSPALine(const std::string &line, ASPAObject &uspa);
};

#endif // RPKI_HPP

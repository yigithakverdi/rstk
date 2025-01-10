#pragma once
#ifndef ASPAOBJ_HPP
#define ASPAOBJ_HPP

#include <set>
#include <string>
#include <vector>

class aspaobj {
public:
  aspaobj();
  aspaobj(const std::string &customer, const std::set<std::string> &providers); // Add this constructor
  ~aspaobj();

  std::string getCustomer() const;
  const std::vector<std::string> &getProviders() const;
  const std::vector<unsigned char> &getSignature() const;

  void setCustomer(const std::string &customer);
  void addProvider(const std::string &provider);
  void setSignature(const std::vector<unsigned char> &signature);

  std::string getCustomer();
  const std::vector<std::string> &getProviders();
  const std::vector<unsigned char> &getSignature();

private:
  std::string customer;
  std::vector<std::string> providers;
  std::vector<unsigned char> signature;
};

#endif // RPKI_HPP

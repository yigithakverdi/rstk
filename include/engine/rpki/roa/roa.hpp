#pragma once

#ifndef ROA_HPP
#define ROA_HPP

#include <string>
#include <set>

class roa {
public:
  roa();
  roa(const std::string &asn, const std::set<std::string> &prefixes); // Add this constructor
  ~roa();
private:
  std::string asn_;
  std::set<std::string> prefixes_;
};

#endif // ROA_HPP

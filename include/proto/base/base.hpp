#pragma once
#ifndef BASE_HPP
#define BASE_HPP

#include "proto/policy.hpp"
#include "proto/proto.hpp"
#include "router/route.hpp"
#include <string>
#include <functional>

class BasePolicy : public IPolicy {
public:
  explicit BasePolicy();
  bool validate(const route &r) const;
  bool prefer(const route &r1, const route &r2) const;
  bool forward(relation source, relation target) const;
  int localPreference(const route &r) const;
  int asPathLength(const route &r) const;
  int nextHopASNumber(const route &r) const;

private:
  enum class rules { localPreference, asPathLength, nextHopASNumber };
  struct config {
    rules rule;
    std::function<int(const route &)> evaluator;
    bool higherIsBetter;
  };
  std::vector<config> rules_;
  void initRules();
};

class BaseProtocol : public IProto {
public:
  BaseProtocol();
  std::string name() const;
  std::string info() const;
};

#endif // BASE_HPP

#pragma once

#ifndef ASPA_HPP
#define ASPA_HPP

#include "engine/rpki/aspa/aspa.hpp"
#include "engine/rpki/rpki.hpp"
#include "proto/policy.hpp"
#include "proto/proto.hpp"
#include "router/route.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class result { valid, invalid, unknown };
enum class auth { provider, nonprovider, noattestation, peer };

class ASPAProtocol;
class ASPAPolicy : public IPolicy {
public:
  explicit ASPAPolicy(std::shared_ptr<rpki> r);
  ASPAPolicy(std::shared_ptr<rpki> r, std::shared_ptr<ASPAProtocol> p);
  bool validate(const route &r) const;
  bool prefer(const route &r1, const route &r2) const;
  bool forward(relation source, relation target) const;
  bool hasASPA(router *r) const;
  bool isProvider(router *r1, router *r2) const;
  result performASPA(const route &r) const;
  int localPreference(const route &r) const;
  int asPathLength(const route &r) const;
  int nextHopASNumber(const route &r) const;

private:
  std::shared_ptr<rpki> rpki_;
  enum class rules { localPreference, asPathLength, nextHopASNumber };
  struct config {
    rules rule;
    std::function<int(const route &)> evaluator;
    bool higherIsBetter;
  };
  std::vector<config> rules_;
  void initRules();
};

class ASPAProtocol : public IProto {
public:
  ASPAProtocol(std::shared_ptr<rpki> r);
  std::string name() const;
  std::string info() const;
  std::vector<aspaobj> getASPAObjects(router *r) const;
  aspaobj getASPAObject(router *r) const;
  aspaobj createASPAObject(router *r) const;
  void publishToRPKI(router *r);
  void addASPAObject(const aspaobj &obj);
  void updateUSPAS();

private:
  std::shared_ptr<rpki> rpki_;
  std::vector<aspaobj> aspaSet_;
};

#endif // ASPA_HPP

#pragma once

#ifndef ASCONES_HPP
#define ASCONES_HPP

#include "engine/rpki/rpki.hpp"
#include "plugins/plugins.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include <memory>
#include <string>

enum class ASConesResult { Valid, Invalid, Unknown };

class ASConesPolicyEngine : public PolicyEngine {
public:
  explicit ASConesPolicyEngine(std::shared_ptr<RPKI> rpki);

  bool shouldAcceptRoute(const Route &route) const override;
  bool shouldPreferRoute(const Route &currentRoute,
                         const Route &newRoute) const override;
  bool canForwardRoute(Relation sourceRelation,
                       Relation targetRelation) const override;

  ASConesResult performASConesVerification(const Route &route) const;

private:
  std::shared_ptr<RPKI> rpki_;
  ASConesResult upstreamPathVerification(const Route &route) const;
  ASConesResult downstreamPathVerification(const Route &route) const;
  ASConesResult performVerification(const Route &route) const;
  bool isCustomerAuthorized(Router *currAS, Router *nextAS) const;
};

class ASConesProtocol : public Protocol {
public:
  explicit ASConesProtocol(std::shared_ptr<RPKI> rpki);
  std::string getProtocolName() const override;

private:
  std::shared_ptr<RPKI> rpki_;
};

#endif // ASCONES_HPP

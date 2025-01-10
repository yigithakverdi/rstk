#pragma once
#ifndef PROTOCOLS_HPP
#define PROTOCOLS_HPP

#include "policy.hpp"
#include <memory>
#include <string>

class IProto {
public:
  explicit IProto(std::unique_ptr<IPolicy> policy);
  virtual ~IProto();
  bool accept(const route &r) const;
  bool prefer(const route &r1, const route &r2) const;
  bool forward(const std::string &source, const std::string &target) const;

  virtual std::string name() const;
  virtual std::string info() const;

private:
  std::unique_ptr<IPolicy> policy_;
};

#endif // PROTOCOLS_HPP

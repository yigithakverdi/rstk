#pragma once
#ifndef POLICY_HPP
#define POLICY_HPP

class route;
class topology;
enum class relation;

class IPolicy {
public:
  virtual ~IPolicy();
  virtual bool validate(const route& r) const;
  virtual bool prefer(const route& r1, const route& r2) const;
  virtual bool forward(relation source, relation target) const;
};


#endif // POLICY_HPP

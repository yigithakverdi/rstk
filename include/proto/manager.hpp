// manager.hpp
#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <string>

class IProto;
class IPolicy;

class protofactory {
public:
  protofactory(const protofactory &) = delete;
  protofactory &operator=(const protofactory &) = delete;
  static protofactory &instance();
  IProto *create(const std::string &name) const;

private:
  protofactory() = default;
};

#endif // MANAGER_HPP

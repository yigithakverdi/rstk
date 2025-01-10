#include "proto/proto.hpp"
#include "router/router.hpp"

IProto::IProto(std::unique_ptr<IPolicy> policy) : policy_(std::move(policy)) {}
IProto::~IProto() = default;
bool IProto::accept(const route &r) const { return policy_->validate(r); }
bool IProto::prefer(const route &r1, const route &r2) const { return policy_->prefer(r1, r2); }
std::string IProto::name() const { return "base"; }
std::string IProto::info() const { return "Base protocol implementation"; }

bool IProto::forward(const std::string &source, const std::string &target) const {
  static const std::unordered_map<std::string, relation> rel_map = {
      {"customer", relation::customer}, {"peer", relation::peer}, {"provider", relation::provider}};

  auto src = rel_map.find(source);
  auto tgt = rel_map.find(target);

  return policy_->forward(src != rel_map.end() ? src->second : relation::unknown,
                          tgt != rel_map.end() ? tgt->second : relation::unknown);
}

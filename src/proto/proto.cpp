#include "proto/proto.hpp"
#include "router/router.hpp"

IProto::IProto(std::unique_ptr<IPolicy> policy) : policy_(std::move(policy)) {}
IProto::~IProto() = default;
bool IProto::accept(const route &r) const { return policy_->validate(r); }
bool IProto::prefer(const route &r1, const route &r2) const { return policy_->prefer(r1, r2); }
std::string IProto::name() const { return "base"; }
std::string IProto::info() const { return "Base protocol implementation"; }
bool IProto::forward(route *r, relation rel) const { return policy_->forward(r, rel); }

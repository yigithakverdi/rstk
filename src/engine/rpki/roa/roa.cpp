#include "engine/rpki/roa/roa.hpp"

roa::roa() {}
roa::roa(const std::string &asn, const std::set<std::string> &prefixes) : asn_(asn), prefixes_(prefixes) {}
roa::~roa() {}

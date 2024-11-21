package graph

import (
  "rstk/internal/router"
  "rstk/internal/protocols"
)

// To dynamically assign different policy implementations to routers during the initialization 
// process (Init()), you can introduce a PolicyFactory pattern or a similar structure to determine 
// which IPolicy implementation to assign based on the context (e.g., TopologyType, AS number, 
// configuration, etc.).
type PolicyFactory struct {
    DefaultPolicy router.IPolicy
    ASPAPolicy    router.IPolicy
}

func NewPolicyFactory() *PolicyFactory {
    return &PolicyFactory{
        DefaultPolicy: &router.DefaultPolicy{},
        ASPAPolicy:    &router.ASPAPolicy{
            USPAS: make(protocols.USPASTable),
            Neighbors: make(map[int]router.Relation),
            ASNumber: 0,
        }, 
    }
}

func (pf *PolicyFactory) GetPolicy(asNumber int, topologyType string) router.IPolicy {
    switch topologyType {
    case "ASPA":
        return pf.ASPAPolicy
    default:
        return pf.DefaultPolicy
    }
}

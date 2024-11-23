package graph

import (
  "rstk/internal/router"
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
        }, 
    }
}

func (pf *PolicyFactory) GetPolicy(topologyType string, r *router.Router) router.IPolicy {
    switch topologyType {
    case "ASPA":
        return &router.ASPAPolicy{
            Router:    r,
        }
    default:
        return &router.DefaultPolicy{}
    }
}

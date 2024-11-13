package policy 

import (
  "rstk/internal/interfaces"
  "fmt"
  ggraph "github.com/dominikbraun/graph"
)

// Ensure Policy implements interfaces.policy
var _ interfaces.Policy = &Policy{}

// DefaultPolicy implements common logic for routing policies
type Policy struct{
  G ggraph.Graph[string, interfaces.Router]
  T interfaces.Topology
}

func (p *Policy) AcceptRoute(route interfaces.Route) bool {
  if(route.ContainsCycle()) {
    return false
  }
  return true
}

func (p *Policy) PreferRoute(currentRoute interfaces.Route, newRoute interfaces.Route) bool {
	if(currentRoute.GetFinal().GetASNumber() != newRoute.GetFinal().GetASNumber()) {
    fmt.Println("       Routes must have the same final AS")
    panic("       Routes must have the same final as")
  }
  fmt.Println("       Route exists in the routing table prefering routes...")
  fmt.Println("       Comparing routes")
  for index, rule := range p.PreferenceRules() {
    currentVal := rule(currentRoute)
    newVal := rule(newRoute)
    
    if(index == 0) {
      fmt.Println("       Comparing routes based on relationship")
      fmt.Printf("        Current routes, first hop AS: %d, final AS %d relationship: %d\n", currentRoute.GetFirstHop().GetASNumber(), currentRoute.GetFinal().GetASNumber(), *currentVal)
      fmt.Printf("        New routes, first hop AS: %d, final AS %d relationship: %d\n", newRoute.GetFirstHop().GetASNumber(), newRoute.GetFinal().GetASNumber(), *newVal)
    }

    if(index == 1) {
      fmt.Println("       Comparing routes based on AS-Path length")
      fmt.Println("       Current routes AS-Path length: ", *currentVal)
      fmt.Println("       New routes AS-Path length: ", *newVal)
    }

    if(index == 2) {
      fmt.Println("       Comparing routes based on next hop AS number")
      fmt.Println("       Current routes next hop AS number: ", *currentVal)
      fmt.Println("       New routes next hop AS number: ", *newVal)
    }

    // Proceed only if both values are not nil
    if currentVal != nil && newVal != nil {
        if *newVal < *currentVal {
            // newRoute is preferred over currentRoute
            fmt.Println("       New route is preferred over current route")        
            return true
        } else if *currentVal < *newVal {
            // currentRoute is preferred over newRoute
            fmt.Println("       New route is not preferred over current route")
            return false
        }
        // If equal, continue to the next rule
        fmt.Println("       Routes are equal based on this rule")
        fmt.Println("       Proceeding to the next rule")
    }    
  }
  return false
}

// PreferenceRules returns a list of rules for preference
func (p *Policy) PreferenceRules() []func(interfaces.Route) *int {
  t := p.GetTopology()
  return []func(interfaces.Route) *int{
		
    // Local preference
    func(route interfaces.Route) *int {
      rFirstHop := route.GetFirstHop()
      rFinalAS := route.GetFinal()

      relation, err := t.GetRelationships(rFirstHop, rFinalAS)
      if err != nil {
        fallback := -1
        return &fallback
      }
      val := int(relation)
      return &val  
    },

		// AS-Path length
    func(route interfaces.Route) *int {
		  // fmt.Println("AS-Path length")
      val := len(route.GetPath())
			return &val
		},

    // Next hop AS number
		func(route interfaces.Route) *int {
      // Using initial character as a placeholder temporarly
      // fmt.Println("Next hop AS number")
      val := int(route.GetFirstHop().GetASNumber())
      return &val
    },
	}
}

func (p *Policy) ForwardTo(route interfaces.Route, neighborRelation interfaces.Relation) bool {
    // Determine the relation from which the route was received
    t := p.GetTopology()
    currentAS := route.GetPath()[1]
    previousAS := route.GetPath()[len(route.GetPath())-2] // The AS before the current one

    relationship, err := t.GetRelationships(currentAS, previousAS)
    if err != nil {
        return false
    }

    if relationship == interfaces.Customer {
        // Routes learned from customers can be advertised to anyone
        return true
    } else if relationship == interfaces.Peer || relationship == interfaces.Provider {
        // Routes learned from peers or providers can only be advertised to customers
        return neighborRelation == interfaces.Customer
    }

    return false
}


// GetTopology returns the topology used by the Policy
func (p *Policy) GetTopology() interfaces.Topology {
  return p.T
}

// // Policy for handling PBR based on packet attributes
// type PBRPolicy struct {
// 	DefaultPolicy
// 	PacketCriteria map[string]string // Key-value pairs for matching packet criteria
// }
//
// func (p *PBRPolicy) AcceptRoute(route router.Route) bool {
// 	// Check if the route matches the specified packet criteria for PBR
// 	return p.matchesPacketCriteria(route.Packet) && p.DefaultPolicy.AcceptRoute(route)
// }
//
// func (p *PBRPolicy) matchesPacketCriteria(packet router.Packet) bool {
// 	// Check each criterion in router.PacketCriteria
// 	for key, value := range p.PacketCriteria {
// 		switch key {
// 		case "SourceAddr":
// 			if packet.SourceAddr != value {
// 				return false
// 			}
// 		case "DestinationAddr":
// 			if packet.DestinationAddr != value {
// 				return false
// 			}
// 		case "Size":
// 			if packet.Size != int(value[0]-'0') { // Simplified for example
// 				return false
// 			}
// 		case "Protocol":
// 			if packet.Protocol != value {
// 				return false
// 			}
// 		}
// 	}
// 	return true
// }
//
// // Custom routing policy implementations
// type RPKIPolicy struct {
// 	DefaultPolicy
// }
//
// func (rp *RPKIPolicy) AcceptRoute(route router.Route) bool {
// 	return rp.DefaultPolicy.AcceptRoute(route) && !route.OriginInvalid
// }

//Helper functions to evaluate routing against policies
// TODO for now just a placeholder implementation

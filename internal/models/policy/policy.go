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

  for _, rule := range p.PreferenceRules() {
    currentVal := rule(currentRoute)
    newVal := rule(newRoute)

    fmt.Println("       Current value: ", &currentVal)
    fmt.Println("       New value: ", &newVal)

    // Proceed only if both values are not nil
    if currentVal != nil && newVal != nil {
        if *newVal < *currentVal {
            // newRoute is preferred over currentRoute
            return true
        } else if *currentVal < *newVal {
            // currentRoute is preferred over newRoute
            return false
        }
        // If equal, continue to the next rule
    }    
  }
  return false
}

// func (p *Policy) ForwardTo(route interfaces.Route, relation interfaces.Relation) bool {
// 	// Forwarding logic based on relationships
//
//   t := p.GetTopology()
//
//   rFirstHop := route.GetFirstHop()
//   rFinalAS := route.GetFinal()
//
//   relationship, err := t.GetRelationships(rFirstHop, rFinalAS) 
//
//   if err != nil 
//     return false
//   }
//
//   // Route is forwarded either if was recieved by a customer or if it is going to be sen to a customer
//   return int(relationship) == int(interfaces.Customer) || int(relation) == int(interfaces.Customer)
// }

func (p *Policy) ForwardTo(route interfaces.Route, neighborRelation interfaces.Relation) bool {
    // Determine the relation from which the route was received
    t := p.GetTopology()
    currentAS := route.GetFirstHop()
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

// PreferenceRules returns a list of rules for preference
func (p *Policy) PreferenceRules() []func(interfaces.Route) *int {
  t := p.GetTopology()
  return []func(interfaces.Route) *int{
		// Local preference
    func(route interfaces.Route) *int {
      fmt.Println("Local preference")
      rFirstHop := route.GetFirstHop()
      rFinalAS := route.GetFinal()

      relation, err := t.GetRelationships(rFirstHop, rFinalAS)
      if err != nil {
        return nil    
      }
      // TODO Logic here could be wrong
      val := int(relation)
      return &val  
    },

		// AS-Path length

    func(route interfaces.Route) *int {
		  fmt.Println("AS-Path length")
      val := len(route.GetPath())
			return &val
		},

    // Next hop AS number
		func(route interfaces.Route) *int {
      // Using initial character as a placeholder temporarly
      fmt.Println("Next hop AS number")
      val := int(route.GetFirstHop().GetASNumber())
      return &val
    },
	}
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

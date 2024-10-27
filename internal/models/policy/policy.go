package router 

import (
  "rstk/internal/engine/graph"
  "rstk/internal/interfaces"
  ggraph "github.com/dominikbraun/graph"
)

// DefaultPolicy implements common logic for routing policies
type DefaultPolicy struct{
  g ggraph.Graph[string, interfaces.Router]
}

func (dp *DefaultPolicy) AcceptRoute(route interfaces.Route) bool {
  if(route.ContainsCycle()) {
    return false
  }
  return true
}

func (dp *DefaultPolicy) PreferRoute(currentRoute interfaces.Route, newRoute interfaces.Route) bool {
	if(currentRoute.GetFinal().GetASNumber() != newRoute.GetFinal().GetASNumber()) {
    panic("Routes must have the same final as")
  }

  for _, rule := range dp.PreferenceRules() {
    currentVal := rule(currentRoute)
    newVal := rule(newRoute)

    if(currentVal != nil) {
      if(newVal == nil) {
        if(*currentVal < *newVal) {
          return false
        }
        if(*newVal < *currentVal) {
          return true
        }
      }
    }
  }
  return false
}

func (dp *DefaultPolicy) ForwardTo(route interfaces.Route, relation interfaces.Relation) bool {
	// Forwarding logic based on relationships

  rFirstHop := route.GetFirstHop()
  rFinalAS := route.GetFinal()

  relationship, err := graph.GetRelationship(dp.g, rFirstHop, rFinalAS) 

  if err != nil {
    return false
  }

  // Route is forwarded either if was recieved by a customer or if it is going to be sen to a customer
  return int(relationship) == int(interfaces.Customer) || int(relation) == int(interfaces.Customer)
}

// PreferenceRules returns a list of rules for preference
func (dp *DefaultPolicy) PreferenceRules() []func(interfaces.Route) *int {
	return []func(interfaces.Route) *int{
		// Local preference
    func(route interfaces.Route) *int {

      rFirstHop := route.GetFirstHop()
      rFinalAS := route.GetFinal()

      relation, err := graph.GetRelationship(dp.g, rFirstHop, rFinalAS)
      if err != nil {
        return nil    
      }
      // TODO Logic here could be wrong
      val := int(relation)
      return &val  
    },

		// AS-Path length
    func(route interfaces.Route) *int {
			val := len(route.GetPath())
			return &val
		},

    // Next hop AS number
		func(route interfaces.Route) *int {
      // Using initial character as a placeholder temporarly
      val := int(route.GetFirstHop().GetASNumber())
      return &val
    },
	}
}
//
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

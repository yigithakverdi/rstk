package router 

import (
  "rstk/internal/interfaces"
)

const (
    Customer interfaces.Relation = -1
    Peer     interfaces.Relation = 0
    Provider interfaces.Relation = 1
)

// Router model, could be AS, could be inra-AS router, could be a clinet
// defined for broad capturing of the routing operations
type Interface struct {
    Name string
    IP   string
}

type Packet struct {
    SourceAddr      string
    DestinationAddr string
    Size            int
    Protocol        string
}

type Route struct {
  FinalAS         interfaces.Router 
  FirstHopAS      interfaces.Router 
  Path            []interfaces.Router
  
  PathLength      int
	Authenticated   bool
	OriginInvalid   bool
  PathEndInvalid  bool
  Packet          Packet 
}

func (r* Route) GetPath() []interfaces.Router {
  return r.Path
}

func (r* Route) ContainsCycle() bool {
  path := r.Path
  visited := make(map[int]bool)
  for _, router := range path {
    if visited[router.GetASNumber()] {
      return true
    }
    visited[router.GetASNumber()] = true
  }
  return false
}
func (r *Route) Length() int {
  return r.PathLength
}

func (r *Route) Origin() interfaces.Router {
  if(len(r.Path) > 0) {
    return r.GetPath()[0]
  }
  return nil
}

func (r *Route) GetAuthenticated() bool {
  return r.Authenticated
}

func (r *Route) GetFinal() interfaces.Router {
  if(len (r.Path) > 0) {
    return r.GetPath()[len(r.Path) - 1]
  }
  return nil 
}

func (r *Route) GetFirstHop() interfaces.Router {
  return r.Path[1]
}

func (r *Route) GetFinalAS() interfaces.Router {
  return r.FinalAS
}

func (r *Route) IsAuthenticated() bool {
  return r.Authenticated
}

func (r *Route) IsOriginInvalid() bool {
  return r.OriginInvalid
}

func (r *Route) IsPathEndInvalid() bool {
  return r.PathEndInvalid
}

type Router struct {
  Providers  []interfaces.Router
  Peers      []interfaces.Router
  Customers  []interfaces.Router
  Interfaces []interfaces.Relation
  Policy     interfaces.Policy
  Neighbors  []interfaces.Neighbor
  RouteTable map[int]interfaces.Route

	ID         string
  RouterType string
  ASNumber   int
	RouterID   string
  isCritical bool

}

func (r *Router) GetPolicy() interfaces.Policy {
  return r.Policy
}

func (r *Router) GetRouteTable() map[int]interfaces.Route {
  return r.RouteTable
}

// func (r *Router) GetPacket() Packet {
//   return Packet(r.RouteTable[len(r.RouteTable) - 1].Packet)
// }

func (r *Router) GetASNumber() int {
  return r.ASNumber
}

func (r *Router) IsCritical() bool {
  return r.isCritical
}

func (r *Router) ResetRouteTable() {
  r.RouteTable = make(map[int]interfaces.Route)
}

func (r *Router) ForceRoute(route interfaces.Route) {
  r.RouteTable[route.GetFinal().GetASNumber()] = route 
}

func (r *Router) LearnRoute(route interfaces.Route) []interfaces.Neighbor {
      // If the route's destination is the router itself, do nothing
    if route.GetFinal().GetASNumber() == r.GetASNumber() {
        return nil
    }

    // Check if the route is acceptable according to the policy
    if !r.Policy.AcceptRoute(route) {
        return nil
    }

    // Check if the new route is preferred over the existing one
    routeTable := r.GetRouteTable()
    existingRoute, exists := routeTable[route.GetFinal().GetASNumber()]
    if exists && !r.Policy.PreferRoute(existingRoute, route) {
        return nil
    }

    // Update the routing table with the new route
    r.RouteTable[route.GetFinal().GetASNumber()] = route

    // Determine which neighbors to advertise the route to
    forwardToRelations := make(map[interfaces.Relation]bool)    
    relations := []interfaces.Relation{interfaces.Provider, interfaces.Peer, interfaces.Customer}
    for _, relation := range relations {
        if r.Policy.ForwardTo(route, relation) {
            forwardToRelations[relation] = true
        }
    }

    // Collect neighbors to advertise to
    neighborsToAdvertise := []interfaces.Neighbor{}
    for _, neighbor := range r.Neighbors {
        if forwardToRelations[neighbor.Relation] {
            neighborsToAdvertise = append(neighborsToAdvertise, neighbor)
        }
    }

    return neighborsToAdvertise
}

// OriginateRoute creates a new Route starting from this router and going to nextHop.
func (r *Router) OriginateRoute(nextHop interfaces.Router) interfaces.Route {
    return &Route{
        Path:            []interfaces.Router{r, nextHop}, // Path includes current router and next hop
        OriginInvalid:   false,
        PathEndInvalid:  false,
        Authenticated:   r.IsCritical(),                  // Set based on whether the current router is critical
    }
}

// ForwardRoute forwards an existing route to the nextHop.
func (r *Router) ForwardRoute(route interfaces.Route, nextHop interfaces.Router) interfaces.Route {
    return &Route{
        FinalAS:         route.GetFinal(),
        Path:            append(route.GetPath(), nextHop),           // Append next hop to the existing path
        OriginInvalid:   route.IsOriginInvalid(),
        PathEndInvalid:  route.IsPathEndInvalid(),
        Authenticated:   route.IsAuthenticated() && nextHop.IsCritical(), // Maintain authentication only if both routers are secure
    }
}

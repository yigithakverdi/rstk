package models

// Router model, could be AS, could be inra-AS router, could be a clinet
// defined for broad capturing of the routing operations
type Router struct {
	ID         string
  RouterType string
  ASNumber   int
	RouterID   string
	Neighbors  []Neighbor
	Interfaces []Interface
  IsCritical bool
  RouteTable []Route
}

type Interface struct {
	Name string
	IP   string
}

type Packet struct {
	SourceAddr   string 
	DestinationAddr string 
	Size         int    
	Protocol     string 
}

type Neighbor struct {
	IP string
	AS int
}

func (r* Router) OrginateRoute(nextHop Router) Route {
  return Route {
    Path : []Router{*r, nextHop},
    OriginInvalid : false,
    PathEndInvalid : false,
    Authenticated : r.IsCritical,
  }
}

func (r* Router) ForwardRoute(route Route, nextHop Router) Route {
  return Route {
    Path : append(route.Path, nextHop),
    OriginInvalid : route.OriginInvalid,
    PathEndInvalid : route.PathEndInvalid,
    Authenticated : route.Authenticated && nextHop.IsCritical,
  }
}

func (r* Router) LearnRoute(route Route) []Router {
  return route.Path
}

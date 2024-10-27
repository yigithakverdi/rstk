package interfaces

type Relation int 


const (
  Customer Relation = -1
  Peer Relation = 0
  Provider Relation = 1
)

type Packet struct {
  SourceAddr      string
  DestinationAddr string
  Size            int
  Protocol        string
}

type Route interface {
  ContainsCycle() bool
  Origin() Router
  GetFirstHop() Router
  GetFinal() Router
  GetPath() []Router
  IsAuthenticated() bool
  IsOriginInvalid() bool
  IsPathEndInvalid() bool
}


type Neighbor struct {
  IP       string
  Relation Relation
  Router    Router
}

type Router interface {
  GetASNumber() int
  LearnRoute(route Route) []Neighbor
  ResetRouteTable()
  ForceRoute(route Route)
  OriginateRoute(nextHop Router) Route
  ForwardRoute(route Route, nextHop Router) Route
  GetRouteTable() map[int]Route
  GetPolicy() Policy
  IsCritical() bool
}

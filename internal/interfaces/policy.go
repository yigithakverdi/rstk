// internal/interfaces/policy.go
package interfaces

type Policy interface {
  AcceptRoute(route Route) bool
  PreferRoute(existingRoute, newRoute Route) bool
  ForwardTo(route Route, relation Relation) bool
  GetTopology() Topology
}


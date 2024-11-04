package interfaces

type Topology interface {
  GetRelationships(node1 Router, node2 Router) (Relation, error)
}

package engine

import (
	"fmt"
	"rstk/internal/graph"
)

type EdgeConfig struct {
	Source      string
	Destination string
	Bandwidth   int
	Latency     int
}

type NodeConfig struct {
	ID       string
	IP       string
	CPU      int
	Memory   int
	Protocol string
}

type GlobalConfig struct {
	Nodes []NodeConfig
	Edges []EdgeConfig
}

type TopologyConfig struct {
	Depth           int
	BranchingFactor int
	Redundancy      bool
}

func GenerateTopology(g graph.Graph) {
	fmt.Println("Generating topology...")

}

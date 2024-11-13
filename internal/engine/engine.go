package engine

import (
	"fmt"
	// "rstk/internal/engine/manager"
	"rstk/internal/parser"
  // "rstk/internal/models/router"
  "rstk/internal/models/policy"
  "github.com/google/uuid"
  "rstk/internal/interfaces"
  // "github.com/edwingeng/deque"
  graph "rstk/internal/engine/graph"
  ggraph "github.com/dominikbraun/graph"
)

// [Global] --> [Simulation] --> [Topology] --> [Node, Edge]

type GlobalConfig struct {
  SimulationConfig SimulationConfig
  TopologyConfig TopologyConfig
	Nodes []NodeConfig
	Edges []EdgeConfig
}

type SimulationConfig struct {
	Topology          TopologyConfig
	SimulationID      string
	KatharaConfigPath string
  AsRelFilePath     string
}

type TopologyConfig struct {
	Depth           int
	BranchingFactor int
	Redundancy      bool
	IPVersion       int
	IPBase          string
	IPMap           map[int]string
}

type EdgeConfig struct {
	Source          string
	Destination     string
	Bandwidth       int
	Latency         int
	CollusionDomain string
}

type NodeConfig struct {
	CPU       int
	Memory    int
	ID        string
	IP        string
	Protocol  string
	Interface string
}

type Engine struct {
  GlobalConfig GlobalConfig
  Topology graph.Topology 
}

// Function for generating a unique simulation ID using UUID.
func (e* Engine) GenerateSimulationID() string {
  return uuid.New().String()
}

func (e* Engine) GetEngine() *Engine {
  return e
}

// Set AS relation file path
func (e* Engine) SetAsRelFilePath(path string) {
  e.GlobalConfig.SimulationConfig.AsRelFilePath = path
}

// Initializes and returns the simulation configuration.
func (e* Engine) InitializeSimulationConfig() SimulationConfig {
  s := SimulationConfig{
		Topology: TopologyConfig{
			Depth:           100,
			BranchingFactor: 10,
			Redundancy:      false,
		},
		SimulationID: e.GenerateSimulationID(),
    AsRelFilePath: "",
	}
  e.GlobalConfig.SimulationConfig = s
  return s
}

// Initialize the topology 
func (e* Engine) InitializeTopolgoy() graph.Topology {
  // Check before if the AS relationship file path is Set
  if e.GlobalConfig.SimulationConfig.AsRelFilePath == "" {
    fmt.Println("AS relationship file path not set")
    return graph.Topology{}
  }

  // Parse AS relationships
  parser := parser.Parser{
      AsRelFilePath:   e.GlobalConfig.SimulationConfig.AsRelFilePath,
      BlacklistTokens: []string{"#"},
  }
  fmt.Printf("Parsing AS relationship file %s\n", e.GlobalConfig.SimulationConfig.AsRelFilePath)
  parser.ParseFile()

  // Create topology and populate graph with AS information parsed from CAIDA file
  t := graph.Topology{}
  
  g, asys := t.PopulateTopology(parser.AsRelationships)
  t.G = g
  t.ASES = asys
  
  t.InitializeAdjacencyMap()
  t.InitializePredecessorMap()
  
  // Set topology for the engine struct as well before returning
  e.Topology = t

  // t.PrintTopology()
  return t
}

func (e* Engine) InitializePolicySettings() graph.Topology {
  t := e.Topology
  for _, as := range t.ASES {
      as.SetPolicy(&policy.Policy{
          T: &t,
      })

      neighbors, err := t.GetNeighbors(as, t.G)
      if err != nil {
          fmt.Println("Error getting neighbors for AS", as.GetASNumber())
          continue
      }

      asNeighbors := []interfaces.Neighbor{}      
      for _, neighbor := range neighbors {
          relation, err := t.GetRelationships(as, neighbor)
          if err != nil {
              fmt.Println("Error getting relationships for AS", as.GetASNumber())
              continue
          }

          asNeighbors = append(asNeighbors, interfaces.Neighbor{
              Router:   neighbor,
              Relation: relation,
          })
      }
      as.Neighbors = asNeighbors
  }

  // Again upadet the topology for the engine struct
  e.Topology = t
  return e.Topology
}

// Generates a topology where all the network elements and data is represented inside nodes, though
// the topology is a subset of the graph, either in equal size or smaller then the original graph
// also the subset generation is controlled by two parameters, the depth (how deep topology from given AS)
// and branching factor (how strecthed topology from the current level)
// Function to get a subset of the graph given a starting AS number, depth, and branching factor
func GenerateTopology(startAS int, g ggraph.Graph[int, int], config TopologyConfig) ggraph.Graph[int, int] {
  maxDepth := config.Depth
  branchingFactor := config.BranchingFactor
  // Create a new empty graph for the subset
  subGraph := ggraph.New(ggraph.IntHash, ggraph.Directed())

  // Track visited vertices to avoid reprocessing them
  visited := make(map[int]bool)
  visited[startAS] = true

  // Add the starting AS to the subgraph
  _ = subGraph.AddVertex(startAS)

  // Perform BFS with depth control and branching factor
  _ = ggraph.BFSWithDepth(g, startAS, func(vertex, depth int) bool {
      // Stop exploring deeper if we have reached the maxDepth
      if depth > maxDepth {
          return true // Stops the BFS traversal for this path
      }

      // Get the adjacency map and limit the number of neighbors based on the branching factor
      adjacencyMap, _ := g.AdjacencyMap()
      neighbors := adjacencyMap[vertex]

      count := 0
      for neighbor := range neighbors {
          if count >= branchingFactor {
              break
          }

          // Only process unvisited neighbors
          if !visited[neighbor] {
              visited[neighbor] = true

              // Add the neighbor to the subgraph
              _ = subGraph.AddVertex(neighbor)

              // Add the edge between the current vertex and the neighbor
              edge, _ := g.Edge(vertex, neighbor)
              _ = subGraph.AddEdge(vertex, neighbor, ggraph.EdgeWeight(edge.Properties.Weight))

              count++
          }
      }

      // Continue exploring
      return false
  })

  return subGraph
}

// Visualize the generated topology
func PrintTopology(g ggraph.Graph[int, int]) {
    // Get the adjacency map
    adjMap, err := g.AdjacencyMap()
    if err != nil {
        fmt.Println("Error getting adjacency map:", err)
        return
    }

    // Iterate over the adjacency map
    for vertex, neighbors := range adjMap {
        fmt.Printf("Vertex %d:\n", vertex)
        for neighbor, edge := range neighbors {
            fmt.Printf("  - connects to %d", neighbor)
            // If you have edge properties, you can print them
            if len(edge.Properties.Attributes) > 0 {
                fmt.Printf(" with attributes: %+v", edge.Properties.Attributes)
            }
            fmt.Println()
        }
    }
}

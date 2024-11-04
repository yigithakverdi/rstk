package main

import (
	"fmt"
	"rstk/internal/engine/graph"
	"rstk/internal/parser"
	"rstk/internal/interfaces"
	"os"
  "rstk/internal/models/policy"
	"runtime/pprof"
)

func main() {
  
  // Create a file to store CPU profile data
  cpuProfile, err := os.Create("logs/cpu.prof")
  if err != nil {
    fmt.Println("Error creating CPU profile file")
    return
  }

  defer cpuProfile.Close() 

  // Start CPU profiling
  if err := pprof.StartCPUProfile(cpuProfile); err != nil {
    fmt.Println("Error starting CPU profile")
    return
  }
  
  // For now using simple topology with 4 ASes with simple customer-provider relationships
  asFile := "data/test-2/test.as-rel2.txt"

  // Parse AS relationships
  parser := parser.Parser{
      AsRelFilePath:   asFile,
      BlacklistTokens: []string{"#"},
  }
  parser.ParseFile()

  // Create topology and populate graph with AS information parsed from CAIDA file
  t := graph.Topology{}
  
  g, asys := t.PopulateGraph(parser.AsRelationships)
  t.G = g
  t.ASES = asys

  t.InitializeAdjacencyMap()
  t.InitializePredecessorMap()

  // t.PrintTopology()

  // Initialize policies and neighbors for all routers
  fmt.Println("Initializing policies and neighbors for all routers")
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

  targetASNumber := 4 
  targetRouter, err := t.GetRouterByASNumber(targetASNumber)
  if err != nil {
    fmt.Println("Error getting target router")
    return
  }

  fmt.Println("Propagating routest from target router")  
  t.FindRoutesTo(targetRouter)

  // Optionally, inspect routing tables or other state.
  for asNumber, router := range t.ASES {
    if(asNumber == 199063) {
      routeTable := router.GetRouteTable()
      fmt.Printf("AS %d routing table:\n", asNumber)
      for destAS, route := range routeTable {
        fmt.Printf("  Destination AS %d via path: ", destAS)
        for _, hop := range route.GetPath() {
            fmt.Printf("%d ", hop.GetASNumber())
        }
        fmt.Println()
      }
    }
  }  
}

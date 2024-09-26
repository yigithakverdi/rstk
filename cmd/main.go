package main

import (
	// "fmt"
	"rstk/internal/engine"
	"rstk/internal/graph"
	"rstk/internal/parser"
)

func main() {
	parser := parser.Parser{
		AsRelFilePath:   "data/serial-2/20151201.as-rel2.txt",
		BlacklistTokens: []string{"#"},
	}

	// Parse the file and store the relationships in the parser struct
	parser.ParseFile()

	// Populate the graph with the relationships
	g := graph.PopulateGraph(parser.AsRelationships)

	// AS number
	asNumber := 1

	// // Using the graph created, test AS neighbor relationships
	// customer, peers, providers := graph.GetNeighbors(as_number)

	// fmt.Printf("AS %d neighbors\n", as_number)
	// printNeighbors := func(label string, neighbors []int) {
	// 	fmt.Printf("%s: ", label)
	// 	if len(neighbors) > 10 {
	// 		fmt.Printf("%v... and %d more\n", neighbors[:10], len(neighbors)-10)
	// 	} else {
	// 		fmt.Println(neighbors)
	// 	}
	// }

	// printNeighbors("Customers", customer)
	// printNeighbors("Peers", peers)
	// printNeighbors("Providers", providers)

	topologyConfig := engine.TopologyConfig{
		Depth:           2,
		BranchingFactor: 1,
		Redundancy:      false,
	}

	engine.GenerateTopology(asNumber, g, topologyConfig)
}

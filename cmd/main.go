package main

import (
	// "rstk/internal/engine/graph"
	// "rstk/internal/parser"
	// "rstk/internal/interfaces"
	//  "rstk/internal/models/policy"
	// "runtime/pprof"
	// "os"
	// "fmt"
  "flag"
  "rstk/cli"
)
 
func main() {

  // Exact file AS relation path
  asRelFilePath := "/home/yigit/workspace/github/rstk/data/test-2/test.as-rel2.txt"

  // Define command-line arguments
  asRelFile := flag.String("as-rel", asRelFilePath, "AS relationship file")
  targetRouter := flag.String("target-router", "199063", "Target router AS number")
  configFile := flag.String("config", "data/config.json", "Configuration file")
  
  // Parse the command-line arguments
  flag.Parse()

  // Remaining arguments after flags are parsed
  remainingArgs := flag.Args()

  // Pass the remaining arguments to the CLI
  cli.Run(*asRelFile, *targetRouter, *configFile, remainingArgs)

  // Create a file to store CPU profile data
  // cpuProfile, err := os.Create("logs/cpu.prof")
  // if err != nil {
  //   fmt.Println("Error creating CPU profile file")
  //   return
  // }

  // defer cpuProfile.Close() 

  // Start CPU profiling
  // if err := pprof.StartCPUProfile(cpuProfile); err != nil {
  //   fmt.Println("Error starting CPU profile")
  //   return
  // }

  // Optionally, inspect routing tables or other state.
  // for asNumber, router := range t.ASES {
  //   if(asNumber == 199063) {
  //     routeTable := router.GetRouteTable()
  //     fmt.Printf("AS %d routing table:\n", asNumber)
  //     for destAS, route := range routeTable {
  //       fmt.Printf("  Destination AS %d via path: ", destAS)
  //       for _, hop := range route.GetPath() {
  //           fmt.Printf("%d ", hop.GetASNumber())
  //       }
  //       fmt.Println()
  //     }
  //   }
  // }  
}

package cli

import (
  "fmt"
  // "rstk/internal/engine"
  // "rstk/internal/models/router"
  "encoding/json"
  "strconv"  
)

func (s* State) GetState() *State {
  return s
}

func (s* State) UpdateState(state *State) {
  s.Engine = state.Engine
  s.TargetRouter = state.TargetRouter
}


func (s* State) Start() {
    fmt.Println("Starting the process...")
    // Additional logic for "start" command
}

func (s* State) Stop() {
    fmt.Println("Stopping the process...")
    // Additional logic for "stop" command
}

func (s* State) Status() {
    fmt.Println("Checking status...")
    // Additional logic for "status" command
}

// func (s* State) Help() {
//     fmt.Println("Available commands:")
//     fmt.Println("  start  - Start the process")
//     fmt.Println("  stop   - Stop the process")
//     fmt.Println("  status - Check the status")
//     fmt.Println("  exit   - Exit the CLI")
// }

func showHelp(commands map[string]Command) {
    fmt.Println("Available commands:")
    for name, cmd := range commands {
        fmt.Printf("  %s - %s\n", name, cmd.Description)
    }
}

// Topology related commands

// Set commmands
func (s* State) SetTargetRouter(args []string) {
  // Check if the correct number of arguments is provided
  if len(args) < 1 {
      fmt.Println("Usage: settargetrouter <AS_number>")
      return
  }  

  // Parse AS number argument
  asNumber, err := strconv.Atoi(args[0])
  if err != nil {
      fmt.Println("Invalid AS number:", args[0])
      return
  }

  // Obtain the state of the CLI
  t := s.Engine.Topology
  
  fmt.Println("Setting target router...")
  targetRouter, err := t.GetRouterByASNumber(asNumber)
  if err != nil {
    fmt.Println("Error getting target router")
    return 
  }
  s.TargetRouter = targetRouter
}

// Initialize commmands
func (s* State) InitializeTopology() {
  fmt.Println("Initializing topology...")
  // s.Engine.InitializeSimulationConfig()
  s.Engine.InitializeTopolgoy()
  s.Engine.InitializePolicySettings()
}

// func (s* State) InitializeSimulationConfig() {
//   fmt.Println("Initializing simulation configuration...")
//   s.Engine.InitializeSimulationConfig()
//   s.Engine.InitializeTopolgoy()
// }

// func (s* State) InitializePolicySettings() {
//   fmt.Println("Initializing policy settings...")
//   s.Engine.InitializePolicySettings()
// }

// Routing commands
func (s* State) FindRoutesTo(args []string) {
  // Check if the correct number of arguments is provided
  if len(args) < 1 {
      fmt.Println("Usage: settargetrouter <AS_number>")
      return
  }  

  // Parse AS number argument
  asNumber, err := strconv.Atoi(args[0])
  if err != nil {
      fmt.Println("Invalid AS number:", args[0])
      return
  }

  // Obtain the state of the CLI
  t := s.Engine.Topology
  
  fmt.Println("Setting target router...")
  targetRouter, err := t.GetRouterByASNumber(asNumber)
  if err != nil {
    fmt.Println("Error getting target router")
    return 
  }  
  
  fmt.Println("Propagating routes from target router")
  t.FindRoutesTo(targetRouter)
}

// Set commands
func (s* State) SetAsRelFilePath(args []string) {
  // Check if the correct number of arguments is provided
  if len(args) < 1 {
      fmt.Println("Usage: set-asrel-file <path>")
      return
  }

  // Set the AS relationship file path
  s.Engine.SetAsRelFilePath(args[0])
}

func (s* State) PrintTopology() {
  t := s.Engine.Topology
  fmt.Println("Printing topology...")
  t.PrintTopology()
}

// Get commands

// Config commands
func (s *State) ListConfig() {
    // Prepare a structure to hold all the configurations and CLI state
    output := map[string]interface{}{
        "GlobalConfig":       s.Engine.GlobalConfig,
        // "SimulationConfig":   s.Engine.GlobalConfig.SimulationConfig,
        // "TopologyConfig":     s.Engine.GlobalConfig.SimulationConfig.Topology,
        "TargetRouter":       s.TargetRouter,
        // "SimulationID":       s.Engine.GlobalConfig.SimulationConfig.SimulationID,
        // "KatharaConfigPath":  s.Engine.GlobalConfig.SimulationConfig.KatharaConfigPath,
        // "AsRelFilePath":      s.Engine.GlobalConfig.SimulationConfig.AsRelFilePath,
        "Topology":           s.Engine.Topology,
    }

    // Marshal to JSON
    jsonOutput, err := json.MarshalIndent(output, "", "  ")
    if err != nil {
        fmt.Println("Error formatting configuration as JSON:", err)
        return
    }

    // Print JSON output
    fmt.Println(string(jsonOutput))
}

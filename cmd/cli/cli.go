package cli

import (
  // Core library imports
  "fmt"
  "os"
  "strings"
  
  // Internal library imports
  "rstk/internal/router"
  "rstk/internal/graph"

  // Github library imports
  "github.com/peterh/liner"
)

// CLI has a state, since our simulator have multiple data structure, constnatly
// changing, getting updated, deleted, and since users can interact with these
// data structures, we need to keep track of the state of the simulator, sepcifically
// state of these data structures.
//
// However, to keep the state of the right data structure, we need to handle
// these in a specific way
type State struct {
  // Most important part of the simulator is the topology
  // of the network, all the router, routing, states
  // are stored in the topology data structure
  Topology *graph.Topology

  // The current router that the user is interacting with 
  // user might want to interact with a specific router
  // from topology, we need to also make sure this 
  // router is from the topology, not a random router
  // created out of blue, these checks are done on the
  // related functions
  CurrentRouter *router.Router
}

// Also commands have it's own struct as well, it stores currently function to be
// executed and  description of the command
type Command struct {
	Execute     func(args []string)
	Description string
}


func Run(args []string) {
  
  // Initializing the state of the CLI, with all the necessary data structures
  // critical information needed
  state := State{
    Topology: &graph.Topology{},
  }

  commands := map[string]Command{
    "init-topology": {
      Execute: func(args []string) { state.CInit(args) },
      Description: "Initialize the topology with the given AS relationships",
    },
    "show-topology": {
      Execute: func(args []string) { state.CPrintTpopology() },
      Description: "Show the current topology",
    },
    "show-router": {
      Execute: func(args []string) { state.CGetRouter(args) },
      Description: "Show information about a router",
    },
    "show-relation": {
      Execute: func(args []string) { state.CGetRelation(args) },
      Description: "Show relations of a router",
    },
    "find-routes": {
      Execute: func(args []string) { state.CFindRoutesTo(args) },
      Description: "Find routes to a specific destination",
    },
    "hijack": {
      Execute: func(args []string) { state.CHijack(args) },
      Description: "Hijack a route",
    },
    "add-router": {
      Execute: func(args []string) { state.CAddRouter(args) },
      Description: "Add a router to the topology",
    },
    "create-aspa": {
      Execute: func(args []string) { state.CCreateASPAObject(args) },
      Description: "Create an ASPA policy",
    },
    "generate-aspa": {
      Execute: func(args []string) { state.CGenerateASPAObject(args) },
      Description: "Generate ASPA objects for all routers",
    },
    "show-rpki": {
      Execute: func(args []string) { state.CCheckRPKIStatus() },
      Description: "Show RPKI information",
    },
  }

  // Add help command later on to show available commands
  commands["help"] = Command{
    Execute:     func(args []string) { state.CShowHelp(commands) },
    Description: "Show available commands",
  }

  // Main body of the execution cycle, or main thread for accepting commands etc.
	line := liner.NewLiner()
	defer line.Close()

	// Enable multi-line editing and history support
	line.SetCtrlCAborts(true)
	historyFile := ".cli_history"

	// Load command history if exists
	if f, err := os.Open(historyFile); err == nil {
		line.ReadHistory(f)
		f.Close()
	}

	fmt.Println("Welcome to CLI. Type 'help' for a list of commands.")

	for {
		input, err := line.Prompt("❱ ")
		if err == liner.ErrPromptAborted || input == "exit" {
			fmt.Println("Exiting...")
			break
		} else if err != nil {
			fmt.Println("Error reading line:", err)
			continue
		}

		input = strings.TrimSpace(input)
		line.AppendHistory(input)

		parts := strings.Fields(input)
		if len(parts) == 0 {
			continue
		}

		command := parts[0]
		args := parts[1:]

		if cmd, exists := commands[command]; exists {
			cmd.Execute(args)
		} else {
			fmt.Println("Unknown command. Type 'help' for a list of commands.")
		}
	}

	// Save command history on exit
	if f, err := os.Create(historyFile); err == nil {
		line.WriteHistory(f)
		f.Close()
	}
}

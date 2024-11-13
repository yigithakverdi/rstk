package cli

import (
	// "encoding/json"
	"fmt"
	"os"
	// "strconv"
	"strings"

	"github.com/peterh/liner"
	"rstk/internal/engine"
	"rstk/internal/models/router"
)

type State struct {
	Engine       *engine.Engine
	TargetRouter *router.Router
	AsRelFilePath string
}

type Command struct {
	Execute     func(args []string)
	Description string
}

func Run(asRelFile string, targetRouter string, configFile string, args []string) {
	state := State{
		Engine: &engine.Engine{},
	}

	if asRelFile != "" {
		state.SetAsRelFilePath([]string{asRelFile})
	}

	if targetRouter != "" {
		state.SetTargetRouter([]string{targetRouter})
	}

	commands := map[string]Command{
		"start": {
			Execute:     func(args []string) { state.Start() },
			Description: "Start the process",
		},
		"stop": {
			Execute:     func(args []string) { state.Stop() },
			Description: "Stop the process",
		},
		"status": {
			Execute:     func(args []string) { state.Status() },
			Description: "Check the status",
		},
		// "help": {
		// 	Execute:     func(args []string) { showHelp(commands) },
		// 	Description: "Show available commands",
		// },
		"listconfig": {
			Execute:     func(args []string) { state.ListConfig() },
			Description: "List all configurations in JSON format",
		},
		"settargetrouter": {
			Execute:     state.SetTargetRouter,
			Description: "Set the target router by AS number",
		},
		"exit": {
			Execute:     func(args []string) { fmt.Println("Exiting...") },
			Description: "Exit the CLI",
		},
		"findroutes": {
			Execute:     state.FindRoutesTo,
			Description: "Find routes to the target router",
		},
		"init-topology": {
			Execute:     func(args []string) { state.InitializeTopology() },
			Description: "Initialize the topology",
		},
		"set-asrel-file": {
			Execute:     state.SetAsRelFilePath,
			Description: "Set the AS relationship file path",
		},
		"show-topology": {
			Execute:     func(args []string) { state.PrintTopology() },
			Description: "Show the topology",
		},
	}

  // Add help command later on to show available commands
  commands["help"] = Command{
    Execute:     func(args []string) { showHelp(commands) },
    Description: "Show available commands",
  }

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


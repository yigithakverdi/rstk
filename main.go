package main

import (
  // Core library imports
  // "fmt"
  "flag"

  // Internal library imports
  "rstk/cmd/cli"

  // Github library imports
  log "github.com/sirupsen/logrus"

)

func main() {

  // Set the log level
  log.SetLevel(log.DebugLevel)

  // Customize the log format if desired
  log.SetFormatter(&log.TextFormatter{
    FullTimestamp: true,
  })

  // Parse the flags
  remaingArgs := flag.Args()

  // Main entry point for CLI application
  cli.Run(remaingArgs)
}

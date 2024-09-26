package manager

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
)

var (
	SimulationArtifactPath = "/var/lib/rstk/artifacts/"
	TempDirectory          = "/var/lib/rstk/temp/"
	LogFilePath            = "/var/log/rstk/"
	// DefaultFilePerm        = os.FileMode(0644)
	// MaxFileSize            = 10 * 1024 * 1024
)

// The init function is called automatically when the package is loaded.
// func init() {
// 	logFile, err := os.OpenFile(LogFilePath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, DefaultFilePerm)
// 	if err != nil {
// 		log.Fatalf("Failed to open log file: %v", err)
// 	}
// 	log.SetOutput(logFile)
// 	log.Println("Logging initialized")
// }

// Function for creating simulation directories for the given simulation ID. Every simulation
// artifacts are stored in a directory with the simulation ID as the name. Returns
// the path to the created directory.
func CreateSimulationDirectory(simulationID string) (string, error) {
	simulationPath := filepath.Join(SimulationArtifactPath, simulationID)
	err := os.Mkdir(simulationPath, os.ModePerm)
	if err != nil {
		log.Printf("Failed to create simulation directory %s: %v", simulationPath, err)
		return "", err
	}
	log.Printf("Successfully created simulation directory: %s", simulationPath)
	return simulationPath, nil
}

// Function for creating Kathara configuration file, name of the file is the simulation ID.
// It returns the path to the created configuration file.
func CreateKatharaConfigFile(simulationID string) (string, error) {
	configFilePath := filepath.Join(SimulationArtifactPath, simulationID, simulationID+".conf")
	err := createFile(configFilePath)
	if err != nil {
		log.Printf("Failed to create configuration file %s: %v", configFilePath, err)
		return "", fmt.Errorf("failed to create configuration file %s: %w", configFilePath, err)
	}
	log.Printf("Successfully created configuration file: %s", configFilePath)
	return configFilePath, nil
}

// Function for creating a file wrapper. It returns an error if the file
// creation fails.
func createFile(filePath string) error {
	file, err := os.Create(filePath)
	if err != nil {
		return fmt.Errorf("failed to create file %s: %w", filePath, err)
	}
	defer file.Close()
	return nil
}

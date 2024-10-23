package manager

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
	// "rstk/internal/graph"
)

var (
	SimulationArtifactPath = "/var/lib/rstk/artifacts/"
	TempDirectory          = "/var/lib/rstk/temp/"
	LogFilePath            = "/var/log/rstk/"
	// DefaultFilePerm        = os.FileMode(0644)
	// MaxFileSize            = 10 * 1024 * 1024
)

// Init function for preparing the required directories and files for the simulation.
func Init() {
	log.Printf("Initializing file manager")
	err := createDirectories()
	if err != nil {
		log.Fatalf("Failed to create directories: %v", err)
	}
}

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
	configFilePath := filepath.Join(SimulationArtifactPath, simulationID+"/kathara.conf")
	err := createFile(configFilePath)
	if err != nil {
		return "", fmt.Errorf("failed to create configuration file %s: %w", configFilePath, err)
	}
	log.Printf("Successfully created configuration file: %s", configFilePath)
	return configFilePath, nil
}

// Create Kathara collusion domain with respect to following taxonomy:
// The syntax is X[Y]=Z, where:
//   - X is the name of a network node
//   - Y is the network interface ethY
//   - Z is the collision domain
//
// It loops over the node's provider, customer, and peer relationships and creates
// collision domain configurations for each of them.
// func CreateCollisionDomains(node graph.Node) map[string]bool {
// 	interfaceID := 0
// 	listOfCollisionDomains := make(map[string]bool)
//
// 	createDomains := func(asList []int) {
// 		for _, as := range asList {
// 			collisionDomain := fmt.Sprintf("%d[%s]=%d", node.ASNumber, fmt.Sprintf("%d", interfaceID), as)
// 			listOfCollisionDomains[collisionDomain] = true
// 			interfaceID++
// 		}
// 	}
//
// 	createDomains(node.Provider)
// 	createDomains(node.Customer)
// 	createDomains(node.Peer)
// 	return listOfCollisionDomains
// }

// Function for writing collision domains to the Kathara configuration file.
// It returns an error if the write operation fails.
func WriteCollisionDomainsToKatharaConfigFile(katharaConfigPath string, collisionDomains map[string]bool) error {
	for domain := range collisionDomains {
		err := writeToFile(katharaConfigPath, domain+"\n")
		if err != nil {
			return fmt.Errorf("failed to write collision domain %s to file %s: %w", domain, katharaConfigPath, err)
		}
	}
	return nil
}

// Function for writing content to a file. It returns an error if the write operation fails.
func writeToFile(filePath string, content string) error {
	file, err := os.OpenFile(filePath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, os.ModePerm)
	if err != nil {
		return fmt.Errorf("failed to open file %s: %w", filePath, err)
	}
	defer file.Close()

	_, err = file.WriteString(content)
	if err != nil {
		return fmt.Errorf("failed to write to file %s: %w", filePath, err)
	}
	return nil
}

// Function for creating a file. It returns an error if the file creation fails.
func createFile(filePath string) error {
	if _, err := os.Stat(filePath); err == nil {
		log.Printf("File %s already exists, not creating", filePath)
		return nil
	} else if !os.IsNotExist(err) {
		return fmt.Errorf("failed to check if file exists %s: %w", filePath, err)
	}

	file, err := os.Create(filePath)
	if err != nil {
		return fmt.Errorf("failed to create file %s: %w", filePath, err)
	}
	defer file.Close()
	return nil
}

// Function for creating the required directories for the simulation.
func createDirectories() error {
	directories := []string{SimulationArtifactPath, TempDirectory, LogFilePath}
	for _, dir := range directories {
		err := os.MkdirAll(dir, os.ModePerm)
		if err != nil {
			return fmt.Errorf("failed to create directory %s: %w", dir, err)
		}
	}
	return nil
}

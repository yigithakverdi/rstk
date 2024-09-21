package parser

// To do this we first infer which AS owns each router independent of the
// interface addresses observed at that router. The ownership inferences
// are based on IP-to-AS mapping derived from public BGP data, list of
// peering prefixes from PeeringDB, and the previously inferred business AS
// relationships. Then we convert the observed IP path into an AS path
// using the router ownership information (rather than mapping each
// observed IP to AS directly) and retain the first AS link in the
// resulting path for the AS graph.
//
// The as-rel files contain p2p and p2c relationships.  The format is:
// <provider-as>|<customer-as>|-1
// <peer-as>|<peer-as>|0|<source>

// Main components of a parser for single AS file
// 1) Read line-by-line
// 2) Validate format
// 3) Sanity check single traceroute for a single AS
// 4) Threshold configurations (depth of traceroute, AS path length, graph size)
// 5) Parse strategy
// 		5.1) Sequential full/partial
//		5.2) Random with max-threshold

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

// Global variables encapsulated into to keep management clean
type Parser struct {
	AsRelFilePath   string
	Data            []byte
	BlacklistTokens map[string]struct{}
}

type TopologySource struct {
	Type     string
	Protocol string
	Date     string
	Source   string
}

// Helper function for checking errors, will streamline error checks since it is major
// when dealing with file io operations
func (p *Parser) check(err error) {
	if err != nil {
		fmt.Println("error:", err)
		return // or handle the error gracefully
	}
}

// Creates a map of string to empty struct, for constant time lookups
// considering the blacklist might grow large
func (p *Parser) isBlacklisted(line string) bool {
	_, ok := p.BlacklistTokens[line]
	return ok
}

// Parse the line that contains AS relations, types protocols dates etc.
func (p *Parser) parseLine(line string) (TopologySource, error) {
	parts := strings.Split(line, "|")
	if len(parts) != 4 {
		return TopologySource{}, fmt.Errorf("invalid line: %s", line)
	}
	return TopologySource{
		Type:     parts[0],
		Protocol: parts[1],
		Date:     parts[2],
		Source:   parts[3],
	}, nil
}

// Reading line-by-line intially for preparing the file to validating, sanity checks and
// configurations stesp
// func (p *Parser) Slurp() {
// 	// Open
// 	file, err := os.Open(p.AsRelFilePath)
// 	p.check(err)

// 	// Ensuring file is properly clsoed when the function exits, regardless of the error
// 	// later in the function.
// 	defer file.Close()

// 	// Read all the bytes from the file, data here is byte slice []byte holds the
// 	// entire content of the file. ReadAll reads all the bytes until EOF
// 	p.Data, err = io.ReadAll(file)
// 	p.check(err)
// 	fmt.Printf("bytes read %d from file %s", len(p.Data), p.AsRelFilePath)
// }

// If the processed files are very large, using io.ReadAll might not be optimal as
// it reads the entire file into memory at once. Instead, processing the file line
// by line using a scanner
func (p *Parser) Slurp() {
	file, err := os.Open(p.AsRelFilePath)
	p.check(err)
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		if !p.isBlacklisted(line) {
			_, err := p.parseLine(line)
			if err != nil {
				fmt.Printf("error parsing line: %v\n", err)
			}
		}
	}

	if err := scanner.Err(); err != nil {
		p.check(err)
	}
}

// Parse the data read from the file, this is the main function that will be called
// after reading the file. Parses it into TopologySource struct
func (p *Parser) Parse() {
	// Converting byte slice to a string and split it into lines
	lines := strings.Split(string(p.Data), "\n")

	// Keeping the skipped lines statistics
	skipped := 0

	// Only do operations on the lines that do not contain blacklisted tokens
	for _, line := range lines {
		if p.isBlacklisted(line) {
			skipped++
			continue
		}

		source, err := p.parseLine(line)
		if err != nil {
			fmt.Printf("error parsing line: %v\n", err)
			continue
		}
		fmt.Printf("source: %v\n", source)
	}

	fmt.Printf("skipped %d lines\n", skipped)
}

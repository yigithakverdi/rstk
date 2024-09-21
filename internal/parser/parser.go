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
//  1. Read line-by-line
//  2. Validate format
//  3. Sanity check single traceroute for a single AS
//  4. Threshold configurations (depth of traceroute, AS path length, graph size)
//  5. Parse strategy
//     5.1) Sequential full/partial
//     5.2) Random with max-threshold

package parser

import (
	// "bufio"
	// "io"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

// Global variables encapsulated into to keep management clean
type Parser struct {
	AsRelFilePath   string
	Data            []byte
	BlacklistTokens []string
	MetaData        []TopologySource
	ASRelationships []ASRelationship
}

type TopologySource struct {
	Type     string
	Protocol string
	Date     string
	Source   string
}

type ASRelationship struct {
	AS1      int
	AS2      int
	Relation int
	Source   string
}

// Helper function for checking errors, will streamline error checks since it is major
// when dealing with file io operations
func check(e error) {
	if e != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", e)
		os.Exit(1)
	}
}

func parseMetaDataLine(line string) (TopologySource, error) {
	parts := strings.Split(line, "|")
	if len(parts) != 5 {
		return TopologySource{}, fmt.Errorf("invalid metadata line format: %s", line)
	}

	parts[0] = strings.TrimPrefix(parts[0], "# source:")

	return TopologySource{
		Type:     parts[0],
		Protocol: parts[1],
		Date:     parts[2],
		Source:   parts[3],
	}, nil
}

func parseASRelationshipLine(line string) (ASRelationship, error) {
	parts := strings.Split(line, "|")

	if len(parts) != 4 {
		return ASRelationship{}, fmt.Errorf("invalid as relationship line format: %s", line)
	}

	as1, err := strconv.Atoi(parts[0])
	if err != nil {
		return ASRelationship{}, fmt.Errorf("invalid as1: %s", parts[0])
	}

	as2, err := strconv.Atoi(parts[1])
	if err != nil {
		return ASRelationship{}, fmt.Errorf("invalid as2: %s", parts[1])
	}

	relation, err := strconv.Atoi(parts[2])
	if err != nil {
		return ASRelationship{}, fmt.Errorf("invalid relation: %s", parts[2])
	}

	return ASRelationship{
		AS1:      as1,
		AS2:      as2,
		Relation: relation,
		Source:   parts[3],
	}, nil
}

// Reading line-by-line intially for preparing the file to validating, sanity checks and
// configurations stesp
func (p *Parser) Slurp() {
	// Open
	file, err := os.Open(p.AsRelFilePath)
	check(err)

	// Ensuring file is properly clsoed when the function exits, regardless of the error
	// later in the function.
	defer file.Close()

	// Read all the bytes from the file, data here is byte slice []byte holds the
	// entire content of the file. ReadAll reads all the bytes until EOF
	p.Data, err = io.ReadAll(file)
	check(err)
	fmt.Printf("bytes read %d from file %s \n", len(p.Data), p.AsRelFilePath)
}

// Parse the data read from the file, this is the main function that will be called
// after reading the file. Parses it into TopologySource and ASRelationship structs
func (p *Parser) Parse() *Parser {
	// Converting byte slice to a string and split it into lines
	lines := strings.Split(string(p.Data), "\n")

	for _, line := range lines {

		// Skip empty lines
		if len(line) == 0 {
			continue
		}

		// Skip lines containing blacklisted tokens
		blacklisted := false
		for _, token := range p.BlacklistTokens {
			if strings.Contains(line, token) {
				blacklisted = true
				break
			}
		}
		if blacklisted {
			continue
		}

		if strings.HasPrefix(line, "#") {
			metaData, err := parseMetaDataLine(line)
			check(err)
			p.MetaData = append(p.MetaData, metaData)
		} else {
			asRelationship, err := parseASRelationshipLine(line)
			check(err)
			p.ASRelationships = append(p.ASRelationships, asRelationship)
		}
	}

	return p
}

// The 'serial-2' directory combines the 'serial-1' data with relationships
// inferred from Ark traceroute data, and multi-lateral peering
// https://catalog.caida.org/paper/2013_inferring_multilateral_peering.

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
	"bufio"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

// Parser struct to encapsulate parser data and configuration
type Parser struct {
	AsRelFilePath   string
	AsRelationships []AsRel
	BlacklistTokens []string
}

// TopologySource struct representing the structure of parsed data (optional for future use)
type TopologySource struct {
	Type     string
	Protocol string
	Date     string
	Source   string
}

// AsRel struct to represent AS relationships parsed from the file
type AsRel struct {
	AS1      int
	AS2      int
	Relation int
	Source   string
}

// Helper function for error checking
func (p *Parser) check(err error) error {
	if err != nil {
		return fmt.Errorf("error: %v", err)
	}
	return nil
}

// Check if line is blacklisted based on exact matches or substring matches
func (p *Parser) isBlacklisted(line string) bool {
	for _, token := range p.BlacklistTokens {
		if strings.Contains(line, token) {
			return true
		}
	}

	return false
}

// Parse a single line that contains AS relations, handling error reporting
func (p *Parser) parseLine(line string) (AsRel, error) {
	parts := strings.Split(line, "|")
	if len(parts) != 4 {
		return AsRel{}, p.check(fmt.Errorf("invalid line format: %s", line))
	}

	as1, err := strconv.Atoi(parts[0])
	if err != nil {
		return AsRel{}, p.check(fmt.Errorf("invalid AS1: %s", parts[0]))
	}

	as2, err := strconv.Atoi(parts[1])
	if err != nil {
		return AsRel{}, p.check(fmt.Errorf("invalid AS2: %s", parts[1]))
	}

	relation, err := strconv.Atoi(parts[2])
	if err != nil {
		return AsRel{}, p.check(fmt.Errorf("invalid Relation: %s", parts[2]))
	}

	source := parts[3]

	return AsRel{
		AS1:      as1,
		AS2:      as2,
		Relation: relation,
		Source:   source,
	}, nil
}

// Parse input from an io.Reader (for example, a file or string)
func (p *Parser) Parse(r io.Reader) error {
	scanner := bufio.NewScanner(r)
	for scanner.Scan() {
		line := scanner.Text()
		if !p.isBlacklisted(line) {
			asRel, err := p.parseLine(line)
			if err != nil {
				return fmt.Errorf("error parsing line: %v", err)
			}
			p.AsRelationships = append(p.AsRelationships, asRel)
		}
	}

	if err := scanner.Err(); err != nil {
		return fmt.Errorf("scanner error: %v", err)
	}

	return nil
}

// Open and parse file directly by delegating to Parse with os.Open
func (p *Parser) ParseFile() error {
	file, err := os.Open(p.AsRelFilePath)
	if err != nil {
		return fmt.Errorf("error opening file: %v", err)
	}
	defer file.Close()

	return p.Parse(file)
}

// Function for parsing from a string (useful for unit tests)
func (p *Parser) ParseFromString(data string) error {
	return p.Parse(strings.NewReader(data))
}

// Function to retrieve the parsed relationships
func (p *Parser) GetParsedAsRels() []AsRel {
	return p.AsRelationships
}

// Sanity checking single traceroute for a single AS
func (p *Parser) SanityCheckSingleTraceroute(as int) error {
	return nil
}

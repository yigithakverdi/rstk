package parser 

import (
  "strings"
  "strconv"
  "fmt"
  "os"
  "bufio"
)

// Struct for stroing AS relationships in a meaningful way so that we can easily access 
// which AS we want and their relationships, and store them instances in a list
type AsRel struct {
	AS1      int
	AS2      int
	Relation int
	Source   string
}

// Helper function for simply error checking, makes the following code 
// more readable
func check(err error) error {
	if err != nil {
		return fmt.Errorf("error: %v", err)
	}
	return nil
}

// Helper function for parsing line by line, extracting AS numbers and relationship. Parse a single 
// line that contains AS relations, handling error reporting
func parseLine(line string) (AsRel, error) {
	parts := strings.Split(line, "|")
	if len(parts) != 4 {
		return AsRel{}, check(fmt.Errorf("invalid line format: %s", line))
	}

	as1, err := strconv.Atoi(parts[0])
	if err != nil {
		return AsRel{}, check(fmt.Errorf("invalid AS1: %s", parts[0]))
	}

	as2, err := strconv.Atoi(parts[1])
	if err != nil {
		return AsRel{}, check(fmt.Errorf("invalid AS2: %s", parts[1]))
	}

	relation, err := strconv.Atoi(parts[2])
	if err != nil {
		return AsRel{}, check(fmt.Errorf("invalid Relation: %s", parts[2]))
	}

	source := parts[3]

	return AsRel{
		AS1:      as1,
		AS2:      as2,
		Relation: relation,
		Source:   source,
	}, nil
}


// Function for parsing AS relationship file specifically CAIDA dataset, and stroring them 
// in a simple sturcture such as <AS1> <AS2> <Relation>, it returns a list of AS relationships
// in that form, takes argument as a file path
func GetAsRelationships(path string) ([]AsRel, error) {
  asRelsList := []AsRel{}
  file, err := os.Open(path)
  if err != nil {
    return nil, fmt.Errorf("error opening file: %v", err)
  }
  defer file.Close()

  scanner := bufio.NewScanner(file)
  for scanner.Scan() {
    line := scanner.Text()
    if(line[0] == '#') {
      continue
    }
    asRel, err := parseLine(line)
    if err != nil {
      return nil, fmt.Errorf("error parsing line: %v", err)
    }

    asRelsList = append(asRelsList, asRel)
  }
  if err := scanner.Err(); err != nil {
    return nil, fmt.Errorf("error reading file: %v", err)
  }
  return asRelsList, nil
}

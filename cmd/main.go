package main

import (
	"fmt"
	"rstk/internal/parser"
)

func main() {
	parser := parser.Parser{
		AsRelFilePath:   "data/serial-2/20151201.as-rel2.txt",
		BlacklistTokens: []string{"#"},
	}

	err := parser.ParseFile()
	if err != nil {
		fmt.Println("Failed to parse file:", err)
	} else {
		asRels := parser.GetParsedAsRels()
		fmt.Println("Parsed AS Relationships:", asRels)
	}

}

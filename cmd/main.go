package main

import (
	"rstk/internal/parser"
)

func main() {

	p := parser.Parser{
		AsRelFilePath:   "data/serial-2/20151201.as-rel2.txt",
		BlacklistTokens: []string{"# input clique:", "# IXP ASes:", "# inferred clique:"},
	}

	p.Slurp()
	p.Parse()
}

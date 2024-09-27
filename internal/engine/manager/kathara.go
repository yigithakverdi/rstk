package manager

import (
	"bytes"
	"fmt"
	"os/exec"
)

func StartKathara() {
	cmd := exec.Command("kathara", "lstart")
	err := cmd.Run()
	var out bytes.Buffer

	cmd.Stdout = &out
	if err != nil {
		fmt.Println(err)
	}
	fmt.Printf("Kathara started: %s", out.String())
}

func StopKathara() error {
	return nil
}

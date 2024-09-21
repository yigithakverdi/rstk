#!/bin/bash

# Directory containing the .txt.bz2 files
DATA_DIR="data/serial-2"

# Find all .txt.bz2 files in the directory and extract them
find "$DATA_DIR" -name "*.txt.bz2" -exec bunzip2 {} \; -exec rm {} \;

echo "Extraction complete."
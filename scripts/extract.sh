#!/bin/bash

# Directory containing the .txt.bz2 files
DATA_DIR="data/serial-2"

# Find all .txt.bz2 files in the directory and extract them
total_files=$(find "$DATA_DIR" -name "*.txt.bz2" | wc -l)
current_file=0

find "$DATA_DIR" -name "*.txt.bz2" -print0 | while IFS= read -r -d '' file; do
    ((current_file++))
    echo "Extracting file $current_file of $total_files: $file"
    bunzip2 "$file"
    echo "$((total_files - current_file)) files left to extract."
done

echo "Extraction complete."
#!/bin/bash

# Exit immediately if a command exits with a non-zero status,
# Treat unset variables as an error, and
# Consider errors in pipelines
set -euo pipefail

# -----------------------------
# Configuration
# -----------------------------

# Define the project root directory
project_root="/home/yigit/workspace/github/rstk-worktree/rstk-refactor"

# Define the base URL to scrape (replace with the actual URL)
base_url="https://publicdata.caida.org/datasets/as-relationships/serial-2/"

# Define directories
download_dir="$project_root/data/caida"
scripts_dir="$project_root/scripts"

# Define a temporary file for storing the downloaded HTML
temp_html=$(mktemp)

# Define a custom User-Agent to bypass robots.txt restrictions
user_agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36"

# -----------------------------
# Setup
# -----------------------------

# Create the download directory if it doesn't exist
mkdir -p "$download_dir"

# Change to the scripts directory
cd "$scripts_dir"

# -----------------------------
# Download the HTML Page
# -----------------------------

echo "Downloading the index page from $base_url ..."
wget --header="User-Agent: $user_agent" -O "$temp_html" "$base_url"

# -----------------------------
# Extract Links
# -----------------------------

echo "Extracting .bz2 links from the downloaded HTML ..."
# Extract href values ending with .bz2, excluding "Parent Directory"
links=$(grep -oP '<a href="\K[^"]+\.bz2' "$temp_html")

# Check if any links were found
if [[ -z "$links" ]]; then
    echo "No .bz2 links found at $base_url."
    rm "$temp_html"
    exit 1
fi

# -----------------------------
# Download Files
# -----------------------------

for link in $links; do
    # Construct the full URL for the file
    file_url="${base_url}${link}"
    
    echo "Downloading $file_url ..."
    
    # Download the file to the download directory with the custom User-Agent
    wget --header="User-Agent: $user_agent" -P "$download_dir" "$file_url"
    
    # Define the full path to the downloaded file
    file_path="$download_dir/$link"
    
    # Check if the file was downloaded successfully
    if [[ ! -f "$file_path" ]]; then
        echo "Failed to download $file_url."
        continue
    fi
    
    # -----------------------------
    # Decompress the File
    # -----------------------------
    
    echo "Decompressing $file_path ..."
    bunzip2 "$file_path"
    
    # Verify decompression
    decompressed_file="${file_path%.bz2}"
    if [[ -f "$decompressed_file" ]]; then
        echo "Successfully decompressed to $decompressed_file."
    else
        echo "Failed to decompress $file_path."
    fi
done

# -----------------------------
# Cleanup
# -----------------------------

# Remove the temporary HTML file
rm "$temp_html"

echo "All files have been downloaded and decompressed successfully."


#!/bin/bash

# URL of the CAIDA AS relationships data
BASE_URL="https://publicdata.caida.org/datasets/as-relationships/serial-2/"

# Download the index.html file
wget -q -O index.html $BASE_URL

# Extract download links from index.html
grep -oP '(?<=href=")[^"]*\.bz2(?=")' index.html | while read -r FILE; do
    echo "Downloading $FILE"
    
    # Download in data/serial-2
    cd data/serial-2

    # Construct the full URL
    FILE_URL="${BASE_URL}${FILE}"
    
    # Download the file
    wget -q $FILE_URL
done

# Clean up
rm index.html
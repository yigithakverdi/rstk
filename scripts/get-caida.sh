#!/bin/bash

# Make data directory if it doesn't exist
if [ ! -d data ]; then
    mkdir data
fi

# URL of the CAIDA AS relationships data
BASE_URL="https://publicdata.caida.org/datasets/as-relationships/serial-2/"

# Download if the the index.html file don't exists
if [ ! -f index.html ]; then
    echo "Downloading index.html"
    wget -q -O index.html $BASE_URL
fi

# Extract download links from index.html
grep -oP '(?<=href=")[^"]*\.bz2(?=")' index.html | while read -r FILE; do
    echo "Downloading $FILE"
    
    # Download in data/serial-2
    cd data/serial-2

    # Construct the full URL
    FILE_URL="${BASE_URL}${FILE}"
    
    # Download the file
    wget $FILE_URL
done

# Go back to root project directory
cd ../..

# Clean up
rm index.html
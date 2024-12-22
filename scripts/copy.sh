#!/bin/bash

# Find all .hpp and .cpp files in the current directory and its subdirectories
find ./../ -name "*.hpp" -o -name "*.cpp" | while read file; do
  # Concatenate the file's content to the clipboard
  xclip -selection clipboard <<< "$(cat "$file")"
  # Optional: Add a newline to separate files in the clipboard
  echo "" | xclip -selection clipboard
done

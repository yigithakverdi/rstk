#pragma once

#include <string>

// Clears the terminal screen
void clearScreen();

// Prompts the user for a single-character choice
char getCharChoice(const std::string &prompt);

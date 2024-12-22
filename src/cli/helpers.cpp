#include "cli/helpers.hpp"
#include <cctype>
#include <iostream>
#include <limits>

void clearScreen() {
  // ANSI escape codes to clear screen and move the cursor to the top
  std::cout << "\033[2J\033[1;1H";
}

char getCharChoice(const std::string &prompt) {
  std::cout << prompt;
  char choice;
  std::cin >> choice;
  // Discard extra characters in the buffer
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  return static_cast<char>(std::tolower(choice));
}

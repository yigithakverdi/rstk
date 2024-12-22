#include "parser/parser.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

void Parser::check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string Parser::trim(const std::string &str) {
  std::string trimmed = str;
  trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
  return trimmed;
}

AsRel Parser::parseLine(const std::string &line) {
  std::istringstream stream(line);
  std::string part;
  std::vector<std::string> parts;

  while (std::getline(stream, part, '|')) {
    parts.push_back(part);
  }

  check(parts.size() == 4, "Invalid line format: " + line);

  int as1 = std::stoi(parts[0]);
  int as2 = std::stoi(parts[1]);
  int relation = std::stoi(parts[2]);
  std::string source = parts[3];

  return {as1, as2, relation, source};
}

std::vector<AsRel> Parser::GetAsRelationships(const std::string &path) {
  std::vector<AsRel> asRelsList;

  std::ifstream file(path);
  check(file.is_open(), "Error opening file: " + path);

  std::string line;
  while (std::getline(file, line)) {
    line = trim(line);

    if (line.empty() || line[0] == '#') {
      continue;
    }

    try {
      AsRel asRel = parseLine(line);
      asRelsList.push_back(asRel);
    } catch (const std::exception &e) {
      std::cerr << "Error parsing line: " << line << "\nReason: " << e.what() << std::endl;
      throw;
    }
  }

  return asRelsList;
}

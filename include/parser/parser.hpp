#include <cctype>
#include <string>
#include <vector>

#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

// Structure to represent AS relationships
struct AsRel {
  int AS1;
  int AS2;
  int Relation;
  std::string Source;
};

class Parser {
public:
  std::vector<AsRel> GetAsRelationships(const std::string &path);  

private:
  void check(bool condition, const std::string &message);
  std::string trim(const std::string &str);
  AsRel parseLine(const std::string &line);
};

#endif // PARSER_HPP

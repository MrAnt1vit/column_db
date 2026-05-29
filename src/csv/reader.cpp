#include "reader.hpp"
#include "../utils/file_utils.hpp"
#include "parser.hpp"
#include <sstream>

namespace columnar {

Schema Reader::readSchema(const std::filesystem::path &path) {
  Schema schema;
  std::string content = read_file(path);
  std::istringstream stream(content);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.empty())
      continue;
    if (line.back() == '\r')
      line.pop_back();

    auto tokens = Parser::parseLine(line, ',');

    if (tokens.size() >= 2) {
      schema.addColumn(tokens[0], stringToType(tokens[1]));
    }
  }
  return schema;
}

} // namespace columnar
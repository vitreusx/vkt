#include <vkt/utils.h>
#include <sstream>

std::vector<char const *> vkMapNames(std::vector<std::string> const &names) {
  return mapV(names, [](std::string const &s) -> auto {
    return s.c_str();
  });
}

std::string readFile(std::ifstream &is) {
  std::stringstream buffer;
  buffer << is.rdbuf();
  return buffer.str();
}
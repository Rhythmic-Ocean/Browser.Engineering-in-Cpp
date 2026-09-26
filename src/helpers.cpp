#include "helpers.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

std::vector<std::string_view> hlp::split(std::string_view str,
                                         std::string delim, size_t nums) {
  std::vector<std::string_view> finalAns{};
  size_t start = 0;
  size_t end;
  size_t i{0};
  while ((end = str.find(delim, start)) != std::string_view::npos && i < nums) {
    finalAns.push_back(str.substr(start, end - start));
    start = end + delim.size();
    ++i;
  }
  finalAns.push_back(str.substr(start));
  return finalAns;
}

std::string_view hlp::strip(std::string_view str) {
  auto start = str.find_first_not_of(" \t\n\r\f\v");
  if (start == std::string_view::npos)
    return "";
  auto end = str.find_last_not_of(" \t\n\r\f\v");
  return str.substr(start, end - start + 1);
}

void hlp::casefold(std::string &stri) {
  for (char &c : stri) {
    c = static_cast<char>(std::tolower(c));
  }
}

void hlp::print_tree(Item *node, int indent) {
  std::string indents(indent, ' ');
  std::cout << indents << *node << std::endl;
  for (auto &child : node->m_children) {
    print_tree(child.get(), indent + 2);
  }
}

std::string hlp::get_default_CSS() {
  std::filesystem::path cssPath =
      std::filesystem::path(PROJECT_ROOT_DIR) / "css" / "Browser.css";
  std::ifstream file(cssPath);
  if (!file.is_open()) {
    throw WindowException("Couldn't open Browser's base CSS template\n");
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

std::vector<Item *> hlp::tree_to_list(Item *node) {
  std::vector<Item *> list{};
  for (auto &child : node->m_children) {
    tree_to_list(child.get());
  }
  return list;
}

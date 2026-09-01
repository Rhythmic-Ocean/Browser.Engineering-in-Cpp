#include "helpers.hpp"
#include <algorithm>
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

std::string hlp::casefold(std::string_view stri) {
  std::string str{stri};
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

#pragma once

#include <climits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class NetworkException : public std::runtime_error {
public:
  explicit NetworkException(const std::string &message)
      : std::runtime_error(message) {}
};

namespace hlp {
std::vector<std::string_view> split(std::string_view str, std::string delim,
                                    size_t nums = INT_MAX);
std::string_view strip(std::string_view str);
std::string casefold(std::string_view str);
} // namespace hlp

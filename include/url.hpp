#pragma once

#include "client.hpp"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class URL {
  std::string m_url{};
  Client m_client{};

public:
  std::string_view scheme{};
  std::string_view host{};
  std::string_view path{};

private:
  void parse();
  void get_response(std::string &response);
  std::unordered_map<std::string, std::string_view>
  parse_response(std::vector<std::string_view> response, int &indx);

public:
  explicit URL(const std::string &url);
  ~URL() = default;
  std::string request();
  friend std::ostream &operator<<(std::ostream &out, const URL &url);
};

std::ostream &operator<<(std::ostream &out, const URL &url);

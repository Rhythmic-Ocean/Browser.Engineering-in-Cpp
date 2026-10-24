#pragma once

#include "client.hpp"
#include "layout.hpp"
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace url {

typedef std::unordered_map<std::string, std::string> Headers;
typedef std::string Body;

class URL {
  Client m_client{};

public:
  std::string m_url{};
  std::string scheme{};
  std::string host{};
  std::string path{};
  std::string m_port{};
  bool is_https{true};

private:
  std::unique_ptr<TTF_Text, Layout::TextDeleter> m_urlText{};
  void parse();
  void get_response(std::string &response);
  void get_http_response(std::string &response);
  std::unordered_map<std::string, std::string>
  parse_response(std::vector<std::string_view> response, size_t &indx);

public:
  URL(const std::string &url);
  URL() = default;
  URL(URL &) = delete;
  URL &operator=(URL &) = delete;
  URL(URL &&) noexcept = default;
  URL &operator=(URL &&) noexcept = default;
  ~URL() = default;
  std::pair<Headers, Body> request(url::URL &referral,
                                   std::string payload = "");
  URL resolve(std::string_view url);
  TTF_Text *get_ttfText(TTF_TextEngine *engine, TTF_Font *font);
  std::string origin() const;

  friend std::ostream &operator<<(std::ostream &out, const URL &url);
};

std::ostream &operator<<(std::ostream &out, const URL &url);

// INFO: CREDIT:
// https://github.com/gofer/percent_encoding/blob/master/src/percent_encoding_encode.cpp
class PercentageEncoding {
  static bool needs_encoding(char ch) {
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
        (ch >= '0' && ch <= '9') || ch == '-' || ch == '.' || ch == '_' ||
        ch == '~')
      return false;
    return true;
  }

public:
  static std::string encode(const std::string &str) {
    std::string buf = "";
    for (auto ch : str) {
      if (ch == ' ') {
        buf += "+";
      } else if (needs_encoding(ch)) {
        int rep = static_cast<int>(static_cast<unsigned char>(ch));
        buf += "%" + std::format("{:02X}", rep);
      } else
        buf += ch;
    }
    return buf;
  }
};
} // namespace url

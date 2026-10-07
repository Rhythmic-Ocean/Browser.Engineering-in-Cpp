#pragma once

#include "client.hpp"
#include "layout.hpp"
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
  std::unordered_map<std::string, std::string_view>
  parse_response(std::vector<std::string_view> response, size_t &indx);

public:
  URL(const std::string &url);
  URL() = default;
  URL(URL &) = delete;
  URL &operator=(URL &) = delete;
  URL(URL &&) noexcept = default;
  URL &operator=(URL &&) noexcept = default;
  ~URL() = default;
  std::string request();
  URL resolve(std::string_view url);
  TTF_Text *get_ttfText(TTF_TextEngine *engine, TTF_Font *font);

  friend std::ostream &operator<<(std::ostream &out, const URL &url);
};

std::ostream &operator<<(std::ostream &out, const URL &url);

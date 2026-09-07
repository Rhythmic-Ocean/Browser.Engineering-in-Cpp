#pragma once

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <climits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct TextDeleter {
  void operator()(TTF_Text *text) const {
    if (text)
      TTF_DestroyText(text);
  }
};

struct DisplayItem {
  std::unique_ptr<TTF_Text, TextDeleter> text_obj;
  float x;
  float y;
  float width;
  float height;
};

struct Item {
  std::string m_text{};
  bool m_tag;
  Item(std::string &text, bool tag) : m_text{text}, m_tag{tag} {}
};

struct myFile {
  void *fontFile;
  size_t file_size;
};

struct EngineDeleter {
  void operator()(TTF_TextEngine *engine) const {
    if (engine)
      TTF_DestroyRendererTextEngine(engine);
    engine = nullptr;
  }
};

struct FileDeleter {
  void operator()(myFile *file) { SDL_free(file->fontFile); }
};

struct FontDeleter {
  void operator()(TTF_Font *font) const { TTF_CloseFont(font); }
};

class NetworkException : public std::runtime_error {
public:
  explicit NetworkException(const std::string &message)
      : std::runtime_error(message) {}
};

class WindowException : public std::runtime_error {
public:
  explicit WindowException(const std::string &message)
      : std::runtime_error(message) {}
};

class FontCacheException : public std::runtime_error {
public:
  explicit FontCacheException(const std::string &message)
      : std::runtime_error(message) {}
};

namespace hlp {
std::vector<std::string_view> split(std::string_view str, std::string delim,
                                    size_t nums = INT_MAX);
std::string_view strip(std::string_view str);
std::string casefold(std::string_view str);
} // namespace hlp
//
inline std::string C_SDL_GetStrError() { return SDL_GetError(); }

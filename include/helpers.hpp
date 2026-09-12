#pragma once

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <climits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

enum class FontStyle { BOLD, ITALICS, BOLD_ITALICS, REGULAR, COUNT };
static constexpr float BASE_FONT_SIZE = 16.0f;
typedef int FontSize;
inline int HSTEP = 13;
inline int VSTEP = 14;

struct TextDeleter {
  void operator()(TTF_Text *text) const {
    if (text)
      TTF_DestroyText(text);
    text = nullptr;
  }
};

struct WindowDeleter {
  void operator()(SDL_Window *window) const {
    if (window)
      SDL_DestroyWindow(window);
    window = nullptr;
  }
};

struct RendererDeleter {
  void operator()(SDL_Renderer *renderer) const {
    if (renderer)
      SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }
};

struct ItemProperties {
  bool breakLine{false};
  int lineSpace{};
};

struct DisplayItem {
  std::unique_ptr<TTF_Text, TextDeleter> text_obj;
  TTF_Font *font{nullptr};
  float x;
  float y;
  float width;
  float height;
  std::unique_ptr<ItemProperties> internal{std::make_unique<ItemProperties>()};
};

enum class ItemType { TAG, TEXT };

struct Item {
  std::string m_text{};
  std::vector<std::unique_ptr<Item>> m_children{};
  Item *m_parent{};
  Item(std::string &text) : m_text{text} {}
  virtual ItemType getType() const = 0;
  virtual ~Item() = default;
};

struct Text : public Item {

  Text(std::string &text) : Item{text} {}
  ItemType getType() const { return ItemType::TEXT; }
};

struct Tag : public Item {

  Tag(std::string &text) : Item{text} {}
  ItemType getType() const { return ItemType::TAG; }
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
  void operator()(myFile *file) {
    SDL_free(file->fontFile);
    file = nullptr;
  }
};

struct FontDeleter {
  void operator()(TTF_Font *font) const {
    TTF_CloseFont(font);
    font = nullptr;
  }
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

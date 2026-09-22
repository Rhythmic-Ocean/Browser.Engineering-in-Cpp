#pragma once

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <climits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

static constexpr float BASE_FONT_SIZE = 16.0f;
typedef int FontSize;
inline int HSTEP = 13;
inline int VSTEP = 14;

static constexpr float WIDTH = 800.0f;
static constexpr float HEIGHT = 600.0f;

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

enum class ItemType { TAG, TEXT };

struct Item {
  std::string m_text{};
  std::vector<std::unique_ptr<Item>> m_children{};
  std::unordered_map<std::string, std::string> m_style{};
  Item *m_parent{};
  Item(std::string &&text, Item *parent)
      : m_text{std::move(text)}, m_parent{parent} {}
  virtual ItemType getType() const = 0;
  virtual std::ostream &printItem(std::ostream &out) = 0;
  virtual ~Item() = default;
  friend std::ostream &operator<<(std::ostream &out, Item &item) {
    return item.printItem(out);
  }
};

struct Text : public Item {

  Text(std::string &&text, Item *parent) : Item{std::move(text), parent} {}
  ItemType getType() const override { return ItemType::TEXT; }
  std::ostream &printItem(std::ostream &out) override {
    out << m_text;
    return out;
  }
};

struct Tag : public Item {
  std::unordered_map<std::string, std::string> m_attributes{};
  Tag(std::string &&text, Item *parent,
      std::unordered_map<std::string, std::string> &&attributes)
      : Item{std::move(text), parent}, m_attributes{std::move(attributes)} {}
  ItemType getType() const override { return ItemType::TAG; }
  std::ostream &printItem(std::ostream &out) override {
    out << "<" + m_text + ">";
    return out;
  }
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
    if (font) {
      TTF_CloseFont(font);
    }
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
void casefold(std::string &str);
void print_tree(Item *node, int indent = 0);
} // namespace hlp
//
inline std::string C_SDL_GetStrError() { return SDL_GetError(); }

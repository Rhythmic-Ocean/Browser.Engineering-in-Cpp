#pragma once

#include "helpers.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace Layout {

static constexpr float HSTEP = 13.0f;
static constexpr float VSTEP = 13.0f;
static constexpr auto BLOCK_ELEMENTS = std::to_array<std::string_view>(
    {"html"sv,    "body"sv,   "article"sv, "section"sv,    "nav"sv,
     "aside"sv,   "h1"sv,     "h2"sv,      "h3"sv,         "h4"sv,
     "h5"sv,      "h6"sv,     "hgroup"sv,  "header"sv,     "footer"sv,
     "address"sv, "p"sv,      "hr"sv,      "pre"sv,        "blockquote"sv,
     "ol"sv,      "ul"sv,     "menu"sv,    "li"sv,         "dl"sv,
     "dt"sv,      "dd"sv,     "figure"sv,  "figcaption"sv, "main"sv,
     "div"sv,     "table"sv,  "form"sv,    "fieldset"sv,   "legend"sv,
     "details"sv, "summary"sv});

enum class FontWeight : std::int32_t {
  BOLD,
  ITALICS,
  BOLD_ITALICS,
  REGULAR,
  COUNT
};
enum class LayoutType { BLOCK, INLINE };

inline SDL_Color parse_color(std::string &name) {
  static const std::unordered_map<std::string, SDL_Color> color_map = {
      {"transparent", {0, 0, 0, 0}},       {"white", {255, 255, 255, 255}},
      {"black", {0, 0, 0, 255}},           {"red", {255, 0, 0, 255}},
      {"green", {0, 128, 0, 255}},         {"blue", {0, 0, 255, 255}},
      {"gray", {128, 128, 128, 255}},      {"grey", {128, 128, 128, 255}},
      {"lightblue", {173, 216, 230, 255}}, {"lightgray", {211, 211, 211, 255}},
      {"orange", {255, 165, 0, 255}},      {"yellow", {255, 255, 0, 255}},
      {"purple", {128, 0, 128, 255}}};

  hlp::casefold(name);
  auto it = color_map.find(name);

  if (it != color_map.end()) {
    return it->second;
  }

  return SDL_Color{0, 0, 0, 255};
}

//--WARNING: Deleters!!
struct TextDeleter {
  void operator()(TTF_Text *text) const {
    if (text)
      TTF_DestroyText(text);
    text = nullptr;
  }
};

class FontCache {

  typedef std::unique_ptr<TTF_Font, FontDeleter> Font;
  typedef std::unique_ptr<myFile, FileDeleter> FontFile;

  std::unordered_map<FontWeight, std::pair<FontFile, FontFile>> fontFile_map{};
  std::vector<std::unordered_map<FontSize, Font>> font_vec{
      static_cast<int>(FontWeight::COUNT)};

  void init_fontFiles();
  void init_normalFonts();
  void load_fontFiles(FontWeight weight, const std::string &primary,
                      const std::string &fallback);
  void load_font(FontWeight weight, FontSize size);

public:
  static FontWeight get_weight(const std::string &str) {
    if (str == "bold")
      return FontWeight::BOLD;
    if (str == "italics")
      return FontWeight::ITALICS;
    return FontWeight::REGULAR;
  }
  static int get_size(const std::string &str) {
    double fsize = std::stof(str.substr(0, str.size() - 2));
    int isize = static_cast<int>(std::round(fsize) * 10);
    return isize;
  }

  FontCache();
  void init();
  TTF_Font *get_font(FontWeight style, FontSize size);
  FontCache(FontCache &) = delete;
  FontCache &operator=(FontCache &) = delete;
  FontCache(FontCache &&) = delete;
  FontCache &operator=(FontCache &&) = delete;
  ~FontCache() = default;
};

struct LayoutContext {
  TTF_TextEngine *textEngine;
  FontCache *fontCache;
  float windowWidth{};
  float windowHeight{};
};

struct PositionedText {
  std::unique_ptr<TTF_Text, TextDeleter> text;
  float start_x{};
  float start_y{};
  std::string color;
  PositionedText(TTF_Text *txt, float x, float y, const std::string &l_color)
      : start_x{x}, start_y{y}, color{std::move(l_color)} {
    text.reset(txt);
  }
};

//--NOTE: Starting public viewing struct/classes
struct DrawItem {
  float m_top{};
  float m_bottom{};
  virtual void execute(float scroll_y, SDL_Renderer *renderer) = 0;
  virtual ~DrawItem() = default;
};

struct DrawText : public DrawItem {

public:
  TTF_Text *m_text;

private:
  TTF_Font *m_font;
  float m_left{};
  std::string color;

public:
  DrawText(TTF_Text *text, float x1, float y1, const std::string &l_color)
      : m_text{text}, m_left{x1}, color{std::move(l_color)} {
    m_top = y1;
    m_font = TTF_GetTextFont(m_text);
    m_bottom = y1 + static_cast<float>((TTF_GetFontLineSkip(m_font)));
  }
  void execute(float scroll_y, SDL_Renderer *renderer) override;
};

struct DrawRect : public DrawItem {
  SDL_FRect rect;
  SDL_Color color;
  DrawRect(float x, float y, float width, float height, std::string &l_color)
      : rect{x, y, width, height}, color{parse_color(l_color)} {
    m_top = y;
    m_bottom = y + height;
  }
  void execute(float scroll_y, SDL_Renderer *renderer) override;
};

class Layout {
public:
  float m_start_x{};
  float m_start_y{};
  float m_width{};
  float m_height{};

  Item *m_node{nullptr};
  Layout *m_parent{nullptr};
  Layout *m_previous{nullptr};
  std::vector<std::unique_ptr<Layout>> m_children{};

  Layout(Item *node, Layout *parent, Layout *previous)
      : m_node{node}, m_parent{parent}, m_previous{previous} {}

  virtual void layout(LayoutContext &ctx) = 0;
  virtual void pprint() = 0;
  virtual std::vector<DrawItem *> paint() = 0;
  virtual ~Layout() = default;
};

class DocumentLayout : public Layout {

public:
  DocumentLayout(Item *node) : Layout{node, nullptr, nullptr} {}
  void layout(LayoutContext &ctx);
  std::vector<DrawItem *> paint();
  void pprint();
  ~DocumentLayout() = default;
};

class BlockLayout : public Layout {
  //--INFO: BlockLayout owns the TTF_Text, NOT DrawText!!!
  std::vector<PositionedText> m_line;
  std::vector<PositionedText> m_displayList;

public:
  float m_cursor_x{};
  float m_cursor_y{};
  FontWeight m_fontWeight =
      FontWeight::REGULAR; // prob make a vector later on cuz
  TTF_Font *m_font{};

private:
  void open_tag(const std::string &tag);
  void process_text(Item *node, LayoutContext &ctx);
  void close_tag(const std::string &tag);
  PositionedText make_display(Item *node, std::string &str, LayoutContext &ctx);
  void recurse(Item *root, LayoutContext &ctx);
  void flush();
  static void getExtremes(int &max_ascent, int &max_descent, int &max_lineskip,
                          std::vector<PositionedText> &line);

public:
  void pprint();
  LayoutType layout_mode();
  BlockLayout(Item *node, Layout *parent, Layout *previous)
      : Layout{node, parent, previous} {}
  void layout(LayoutContext &ctx);
  std::vector<DrawItem *> paint();
  ~BlockLayout() = default;
};

} // namespace Layout

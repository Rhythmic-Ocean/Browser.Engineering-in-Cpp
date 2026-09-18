#pragma once

#include "Browser.hpp"
#include "helpers.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace Layout {

static constexpr float HSTEP = 13.0f;
static constexpr float VSTEP = 13.0f;
static constexpr float BASE_FONT_SIZE = 16.0f;
static constexpr auto BLOCK_ELEMENTS = std::to_array<std::string_view>(
    {"html"sv,    "body"sv,   "article"sv, "section"sv,    "nav"sv,
     "aside"sv,   "h1"sv,     "h2"sv,      "h3"sv,         "h4"sv,
     "h5"sv,      "h6"sv,     "hgroup"sv,  "header"sv,     "footer"sv,
     "address"sv, "p"sv,      "hr"sv,      "pre"sv,        "blockquote"sv,
     "ol"sv,      "ul"sv,     "menu"sv,    "li"sv,         "dl"sv,
     "dt"sv,      "dd"sv,     "figure"sv,  "figcaption"sv, "main"sv,
     "div"sv,     "table"sv,  "form"sv,    "fieldset"sv,   "legend"sv,
     "details"sv, "summary"sv});

enum class FontWeight { BOLD, ITALICS, BOLD_ITALICS, REGULAR, COUNT };
enum class LayoutType { BLOCK, INLINE };

inline FontCache g_fontCache{};

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
  FontCache();
  void init();
  TTF_Font *get_font(FontWeight style, FontSize size);
  FontCache(FontCache &) = delete;
  FontCache &operator=(FontCache &) = delete;
  FontCache(FontCache &&) = delete;
  FontCache &operator=(FontCache &&) = delete;
  ~FontCache() = default;
};

struct PositionedText {
  std::unique_ptr<TTF_Text, TextDeleter> text;
  float start_x{};
  float start_y{};
  PositionedText(TTF_Text *txt, float x, float y) : start_x{x}, start_y{y} {
    text.reset(txt);
  }
};

//--NOTE: Starting public viewing struct/classes
struct DrawItem {
  virtual void execute(float scroll_y, SDL_Renderer *renderer) = 0;
};

struct DrawText : public DrawItem {
private:
  TTF_Text *m_text;
  TTF_Font *m_font;
  float m_left{};
  float m_top{};
  float m_bottom{};

public:
  DrawText(TTF_Text *text, float x1, float y1)
      : m_text{text}, m_left{x1}, m_top{y1} {
    m_font = TTF_GetTextFont(m_text);
    m_bottom = y1 + TTF_GetFontLineSkip(m_font);
  }
  void execute(float scroll_y, SDL_Renderer *renderer) override;
};

struct DrawRect : public DrawItem {
  SDL_FRect rect;
  SDL_Color color;
  DrawRect(float x, float y, float width, float height, std::string &color)
      : rect{x, y, width, height}, color{parse_color(color)} {}
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

  virtual void layout(Browser::LayoutContext &ctx) = 0;
  virtual std::vector<DrawItem *> paint() = 0;
  virtual ~Layout();
};

class DocumentLayout : public Layout {

public:
  DocumentLayout(Item *node) : Layout{node, nullptr, nullptr} {}
  void layout(Browser::LayoutContext &ctx);
  std::vector<DrawItem *> paint();
  ~DocumentLayout() = default;
};

class BlockLayout : public Layout {
  //--INFO: BlockLayout owns the TTF_Text, NOT DrawText!!!
  std::vector<PositionedText> m_line;
  std::vector<PositionedText> m_displayList;

public:
  float m_cursor_x{};
  float m_cursor_y{};
  FontSize m_fontSize = BASE_FONT_SIZE;
  FontWeight m_fontWeight =
      FontWeight::REGULAR; // prob make a vector later on cuz
  TTF_Font *m_font{};

private:
  LayoutType layout_mode();
  void open_tag(const std::string &tag);
  void process_text(const std::string &text, Browser::LayoutContext &ctx);
  void close_tag(const std::string &tag);
  PositionedText make_display(std::string &str, Browser::LayoutContext &ctx);
  void recurse(Item *root, Browser::LayoutContext &ctx);
  void flush();
  static void getExtremes(int &max_ascent, int &max_descent, int &max_lineskip,
                          std::vector<PositionedText> &line);

public:
  BlockLayout(Item *node, Layout *parent, Layout *previous)
      : Layout{node, parent, previous} {}
  void layout(Browser::LayoutContext &ctx);
  std::vector<DrawItem *> paint();
  ~BlockLayout();
};

} // namespace Layout

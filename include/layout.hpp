#pragma once

#include "helpers.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::string_view_literals;

namespace Layout {

static constexpr float HSTEP = 13.0f;
static constexpr float VSTEP = 13.0f;
static constexpr float INPUT_WIDTH_PX = 200.0f;
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
enum class LayoutType { DOCUMENT, BLOCK, INLINE, LINE, TEXT, INPUT };

inline SDL_Color parse_color(std::string name) {
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

struct Rect {
  float left;
  float top;
  float right;
  float bottom;

  Rect(float l_x1, float l_y1, float l_x2, float l_y2)
      : left{l_x1}, top{l_y1}, right{l_x2}, bottom{l_y2} {}
  Rect() = default;
  bool contains_point(float x, float y);
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
  static float get_size(const std::string &str) {
    float fsize = std::stof(str.substr(0, str.size() - 2));
    return fsize;
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
  PositionedText(TTF_Text *txt, float x, float y) : start_x{x}, start_y{y} {
    text.reset(txt);
  }
  PositionedText() = default;

  PositionedText(PositionedText &&) = default;
  PositionedText &operator=(PositionedText &&) = default;
};

enum class DrawType { RECT, TEXT, OUTLINE, LINE };

//--NOTE: Starting public viewing struct/classes
struct DrawItem {
  Rect original{};
  virtual void execute(float scroll_y, SDL_Renderer *renderer) = 0;
  virtual DrawType getType() = 0;
  virtual ~DrawItem() = default;
};

struct DrawText : public DrawItem {

public:
  TTF_Text *m_text;

private:
  TTF_Font *m_font;
  float m_left{};

public:
  DrawType getType() override { return DrawType::TEXT; }
  DrawText(TTF_Text *text, float x1, float y1)
      : m_text{text}, m_font{TTF_GetTextFont(text)}, m_left{x1} {
    int iWidth{};
    TTF_GetTextSize(m_text, &iWidth, nullptr);
    float lineSkip = static_cast<float>(TTF_GetFontLineSkip(m_font));
    float txtWidth = static_cast<float>(iWidth);
    original = Rect{x1, y1, x1 + txtWidth, y1 + lineSkip};
    m_font = TTF_GetTextFont(m_text);
  }
  void execute(float scroll_y, SDL_Renderer *renderer) override;
};

struct DrawRect : public DrawItem {
  SDL_Color color;
  DrawRect(Rect Orect, SDL_Color l_color) : color{l_color} {
    original = std::move(Orect);
  }
  DrawType getType() override { return DrawType::RECT; }
  void execute(float scroll_y, SDL_Renderer *renderer) override;
};

// NOTE: Couldn't find a way to have rectangle with thick outlines thru SDL3, so
// had to improvise a bit here
struct DrawOutline : public DrawItem {
  std::array<SDL_FRect, 4> borders;
  SDL_FRect inner;
  SDL_Color borderColor;
  SDL_Color innerColor;

  DrawOutline(std::array<SDL_FRect, 4> l_borders, SDL_FRect l_inner,
              SDL_Color l_borderColor, SDL_Color l_innerColor)
      : borders{std::move(l_borders)}, inner{l_inner},
        borderColor{l_borderColor}, innerColor{l_innerColor} {}

  DrawType getType() override { return DrawType::OUTLINE; }

  void execute(float scroll_y, SDL_Renderer *renderer) override;

  static std::unique_ptr<DrawOutline> createOutline(Rect Orect,
                                                    SDL_Color innerColor,
                                                    SDL_Color borderColor,
                                                    float thickness) {
    SDL_FRect rect = {Orect.left, Orect.top, Orect.right - Orect.left,
                      Orect.bottom - Orect.top};
    std::array<SDL_FRect, 4> borders = {
        {{rect.x, rect.y, rect.w, thickness},
         {rect.x, rect.y + rect.h - thickness, rect.w, thickness},
         {rect.x, rect.y + thickness, thickness, rect.h - (2.0f * thickness)},
         {rect.x + rect.w - thickness, rect.y + thickness, thickness,
          rect.h - (2.0f * thickness)}}};
    SDL_FRect l_inner = {rect.x + thickness, rect.y + thickness,
                         rect.w - (2.0f * thickness),
                         rect.h - (2.0f * thickness)};
    SDL_Color l_innerColor = innerColor;
    SDL_Color l_borderColor = borderColor;
    auto outline = std::make_unique<DrawOutline>(std::move(borders), l_inner,
                                                 l_borderColor, l_innerColor);
    outline->original = std::move(Orect);
    return outline;
  }
};

// WARNING: Thickness's ignored for a bit, will changed later if things come up
struct DrawLine : public DrawItem {
  float x1;
  float x2;
  float y1;
  float y2;
  SDL_Color color;
  [[maybe_unused]] int thickness; // just gonna ignore the thickness for a bit

  DrawLine(float l_x1, float l_y1, float l_x2, float l_y2, SDL_Color l_color,
           int l_thickness)
      : x1{l_x1}, x2{l_x2}, y1{l_y1}, y2{l_y2}, color{l_color},
        thickness{l_thickness} {
    original = {Rect{x1, y1, x2, y2}};
  }

  DrawType getType() override { return DrawType::LINE; }
  void execute(float scroll_y, SDL_Renderer *renderer) override;
};

class Layout;
class DocumentLayout;
class BlockLayout;
class LineLayout;
class TextLayout;
class InputLayout;

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
  virtual std::vector<std::unique_ptr<DrawItem>> paint() = 0;
  virtual bool should_paint() { return true; }
  virtual LayoutType getType() = 0;
  virtual ~Layout() = default;
};

class DocumentLayout : public Layout {

public:
  DocumentLayout(Item *node) : Layout{node, nullptr, nullptr} {}
  void layout(LayoutContext &ctx);
  std::vector<std::unique_ptr<DrawItem>> paint();
  void pprint();
  LayoutType getType() { return LayoutType::DOCUMENT; }
  ~DocumentLayout() = default;
};

class BlockLayout : public Layout {
  //--INFO: BlockLayout owns the TTF_Text, NOT DrawText!!!

public:
  float m_cursor_x{};
  float m_cursor_y{};

private:
  void word(Item *node, LayoutContext &ctx);
  void input(Item *node, LayoutContext &ctx);
  void make_word(Item *node, std::string str, LayoutContext &ctx);
  void recurse(Item *root, LayoutContext &ctx);
  void flush();
  void new_line();
  bool should_paint() override {
    if (m_node->getType() == ItemType::TEXT)
      return true;
    if (m_node->getType() == ItemType::TAG) {

      if (m_node->m_text != "input" || m_node->m_text != "button")
        return true;
    }
    return false;
  }
  Rect self_rect();

public:
  void pprint() override;
  LayoutType layout_mode();
  //--WARNING: getType() is different form layout_mode()!!
  LayoutType getType() override { return LayoutType::BLOCK; }
  BlockLayout(Item *node, Layout *parent, Layout *previous)
      : Layout{node, parent, previous} {}
  void layout(LayoutContext &ctx) override;
  std::vector<std::unique_ptr<DrawItem>> paint() override;
  ~BlockLayout() = default;
};

class LineLayout : public Layout {
public:
  LineLayout(Item *node, BlockLayout *parent, LineLayout *previous_line);
  void layout(LayoutContext &ctx) override;
  std::vector<std::unique_ptr<DrawItem>> paint() override;
  LayoutType getType() override { return LayoutType::LINE; }
  void getExtremes(float &max_ascent, float &max_descent);
  void pprint() override;
};

class TextLayout : public Layout {
  std::string m_word{};

public:
  TTF_Font *font{};
  std::unique_ptr<TTF_Text, TextDeleter> m_text{};
  TextLayout(Item *node, std::string word, LineLayout *parent,
             TextLayout *previous_word)
      : Layout{node, parent, previous_word}, m_word{std::move(word)} {}

  void layout(LayoutContext &ctx) override;
  std::vector<std::unique_ptr<DrawItem>> paint() override;
  LayoutType getType() override { return LayoutType::TEXT; }
  void pprint() override;
};

// NOTE: Input and TextLayout both live under LineLayout and are treated
// essentially the same way! Both their heights are calculate at
// LineLayout::layout() using TextLayout's text height and InputLayout's
// underlying text's height (which is inherited from Browser.css)

class InputLayout : public Layout {
  std::string m_word{};

public:
  TTF_Font *font{};
  std::unique_ptr<TTF_Text, TextDeleter> m_text{};
  InputLayout(Item *node, LineLayout *parent, TextLayout *previous_word)
      : Layout{node, parent, previous_word} {}
  void layout(LayoutContext &ctx) override;
  std::vector<std::unique_ptr<DrawItem>> paint() override;
  LayoutType getType() override { return LayoutType::INPUT; }
  void pprint() override { std::cout << m_word << std::endl; };
  Rect self_rect() {
    return Rect{m_start_x, m_start_y, m_start_x + m_width,
                m_start_y + m_height};
  }
};

class LayoutException : public std::runtime_error {
public:
  explicit LayoutException(const std::string &message)
      : std::runtime_error(message) {}
};

inline LineLayout::LineLayout(Item *node, BlockLayout *parent,
                              LineLayout *previous_line)
    : Layout{node, parent, previous_line} {}

} // namespace Layout

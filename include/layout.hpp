#pragma once

#include "colorMap.hpp"
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
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorType.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkFontScanner.h>
#include <include/core/SkFontStyle.h>
#include <include/core/SkFontTypes.h>
#include <include/core/SkRRect.h>
#include <include/core/SkRect.h>
#include <include/core/SkScalar.h>
#include <include/core/SkTypeface.h>
#include <include/ports/SkFontConfigInterface.h>
#include <include/ports/SkFontMgr_FontConfigInterface.h>
#include <include/ports/SkFontMgr_fontconfig.h>
#include <include/ports/SkFontScanner_FreeType.h>
#include <iostream>
#include <memory>
#include <print>
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

inline SkColor parseColor(const std::string &color) {
  unsigned int r{}, g{}, b{};
  if (color.starts_with('#') && color.size() == 7) {
    std::from_chars(color.data() + 1, color.data() + 3, r, 16);
    std::from_chars(color.data() + 3, color.data() + 5, g, 16);
    std::from_chars(color.data() + 5, color.data() + 7, b, 16);
  } else if (color.starts_with('#') && color.size() == 9) {
    std::from_chars(color.data() + 1, color.data() + 3, r, 16);
    std::from_chars(color.data() + 3, color.data() + 5, g, 16);
    std::from_chars(color.data() + 5, color.data() + 7, b, 16);
    unsigned int a{};
    std::from_chars(color.data() + 7, color.data() + 9, a, 16);
    return SkColorSetARGB(a, r, g, b);
  } else if (ColorMap::NAMED_COLORS.contains(color)) {
    return parseColor(ColorMap::NAMED_COLORS[color]);
  }
  return SkColorSetRGB(r, g, b);
}

inline SkColor parseColor(std::string_view color) {
  return parseColor(std::string(color));
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

  typedef sk_sp<SkTypeface> Font;
  sk_sp<SkFontMgr> m_fontMgr;
  std::unordered_map<std::string, Font> m_fontMap{};

public:
  static SkFontStyle get_weight(const std::string &str) {
    if (str == "bold")
      return SkFontStyle::Bold();
    else if (str == "italics")
      return SkFontStyle::Italic();
    else if (str == "bold-italics") {
      return SkFontStyle::BoldItalic();
    }
    return SkFontStyle::Normal();
  }
  SkFont get_font(SkFontStyle style, SkScalar size,
                  const std::string &family = "Arial");
  SkFont get_font(SkFontStyle style, const std::string &size,
                  const std::string &family = "Arial");

  static float getUTF8Width(const SkFont &font, const std::string &str) {
    return font.measureText(str.c_str(), str.size(), SkTextEncoding::kUTF8);
  }

  FontCache() {
    m_fontMgr =
        SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
  }
  FontCache(FontCache &) = delete;
  FontCache &operator=(FontCache &) = delete;
  FontCache(FontCache &&) = delete;
  FontCache &operator=(FontCache &&) = delete;
  ~FontCache() = default;
};

struct LayoutContext {
  FontCache *fontCache;
  float windowWidth{};
  float windowHeight{};
};

struct PositionedText {
  std::string text;
  float start_x{};
  float start_y{};
  PositionedText(std::string txt, float x, float y) : start_x{x}, start_y{y} {
    text = std::move(txt);
  }
  PositionedText() = default;

  PositionedText(PositionedText &&) = default;
  PositionedText &operator=(PositionedText &&) = default;
};

enum class DrawType { RECT, TEXT, OUTLINE, LINE };

//--NOTE: Starting public viewing struct/classes
struct DrawItem {
  SkRect rect{};
  virtual void execute(SkCanvas *canvas) = 0;
  virtual DrawType getType() = 0;
  virtual ~DrawItem() = default;
};

struct DrawText : public DrawItem {

  std::string m_text;
  SkColor m_color;
  SkFont m_font;
  SkFontMetrics m_fontMetrics;

  DrawType getType() override { return DrawType::TEXT; }
  DrawText(const std::string &text, const std::string &color, SkFont font,
           float x1, float y1)
      : m_text{text}, m_color{parseColor(color)}, m_font{std::move(font)} {
    float txtWidth{
        m_font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8)};
    [[maybe_unused]] float lineSkip = m_font.getMetrics(&m_fontMetrics);
    rect =
        SkRect::MakeLTRB(x1, y1, x1 + txtWidth,
                         y1 - m_fontMetrics.fAscent + m_fontMetrics.fDescent);
  }
  void execute(SkCanvas *canvas) override;
};

struct DrawRect : public DrawItem {
  SkColor m_color;
  DrawRect(SkRect l_rect, const std::string &color)
      : m_color{parseColor(color)} {
    rect = std::move(l_rect);
  }
  DrawType getType() override { return DrawType::RECT; }
  void execute(SkCanvas *canvas) override;
};

struct DrawRRect : public DrawItem {
  SkRRect rrect{};
  SkColor m_color;
  DrawRRect(SkRect l_rect, float radius, const std::string &color)
      : m_color{parseColor(color)} {
    rect = std::move(l_rect);
    rrect = SkRRect::MakeRectXY(rect, radius, radius);
  }
  DrawType getType() override { return DrawType::RECT; }
  void execute(SkCanvas *canvas) override;
};

struct DrawOutline : public DrawItem {
  SkColor m_color;
  float m_thickness;

  DrawOutline(SkRect l_rect, std::string color, float thickness) {
    rect = std::move(l_rect);
    m_color = parseColor(color);
    m_thickness = thickness;
  }

  DrawType getType() override { return DrawType::OUTLINE; }

  void execute(SkCanvas *canvas) override;
};

struct DrawLine : public DrawItem {
  float m_thickness;
  SkColor m_color;

  DrawLine(float x1, float y1, float x2, float y2, std::string color,
           float thickness)
      : m_thickness{thickness}, m_color{parseColor(color)} {
    rect = {SkRect::MakeLTRB(x1, y1, x2, y2)};
  }

  DrawType getType() override { return DrawType::LINE; }
  void execute(SkCanvas *canvas) override;
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

      if (m_node->m_text == "input" || m_node->m_text == "button")
        return false;
    }
    return true;
  }
  SkRect self_rect();

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
  std::string m_text{};

public:
  SkFont font{};
  SkFontMetrics fontMetrics{};
  TextLayout(Item *node, std::string text, LineLayout *parent,
             TextLayout *previous_word)
      : Layout{node, parent, previous_word}, m_text{std::move(text)} {}

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
  std::string m_text{};
  std::string m_displayText{};

public:
  SkFont font{};
  InputLayout(Item *node, LineLayout *parent, TextLayout *previous_word)
      : Layout{node, parent, previous_word} {}
  void layout(LayoutContext &ctx) override;
  std::vector<std::unique_ptr<DrawItem>> paint() override;
  LayoutType getType() override { return LayoutType::INPUT; }
  void pprint() override { std::cout << m_text << std::endl; };
  SkRect self_rect() {
    return SkRect::MakeLTRB(m_start_x, m_start_y, m_start_x + m_width,
                            m_start_y + m_height);
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

#include "UnicodeDecoder.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkFontTypes.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>
#include <include/core/SkScalar.h>
#include <iostream>

using namespace Layout;

/*--NOTE: DrawItem's member function definitions
          Each item is rendered individually here
*/
void DrawText::execute(SkCanvas *canvas) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(m_color);

  float baseline = rect.top() - m_fontMetrics.fAscent;

  canvas->drawSimpleText(m_text.data(), m_text.size(), SkTextEncoding::kUTF8,
                         rect.left(), baseline, m_font, paint);
}

void DrawRect::execute(SkCanvas *canvas) {
  SkPaint paint;
  paint.setColor(m_color);
  canvas->drawRect(rect, paint);
}

void DrawRRect::execute(SkCanvas *canvas) {
  SkPaint paint;
  paint.setColor(m_color);
  canvas->drawRRect(rrect, paint);
}

void DrawOutline::execute(SkCanvas *canvas) {
  SkPaint paint;
  paint.setColor(m_color);
  paint.setStrokeWidth(m_thickness);
  paint.setStyle(SkPaint::kStroke_Style);
  canvas->drawRect(rect, paint);
}

void DrawLine::execute(SkCanvas *canvas) {
  SkPathBuilder pathBuilder;
  pathBuilder.moveTo(rect.left(), rect.top())
      .lineTo(rect.right(), rect.bottom());
  SkPaint paint;
  paint.setColor(m_color);
  paint.setStrokeWidth(m_thickness);
  paint.setStyle(SkPaint::kStroke_Style);

  canvas->drawPath(pathBuilder.detach(), paint);
}

//--NOTE: Layout's member function definitions

void DocumentLayout::layout(LayoutContext &ctx) {
  BlockLayout *child = new BlockLayout(m_node, this, nullptr);
  m_children.emplace_back(child);
  m_width = ctx.windowWidth - 2 * HSTEP;
  m_start_x = HSTEP;
  m_start_y = VSTEP;
  child->layout(ctx);
  m_height = child->m_height;
}

std::vector<std::unique_ptr<DrawItem>> DocumentLayout::paint() { return {}; }

LayoutType BlockLayout::layout_mode() {
  auto checkBlockTag = [](std::vector<std::unique_ptr<Item>> &children) {
    for (auto &child : children) {
      if (std::ranges::find(BLOCK_ELEMENTS, child->m_text) !=
          BLOCK_ELEMENTS.end())
        return true;
    }
    return false;
  };
  // if plain text, it's inline by default
  if (m_node->getType() == ItemType::TEXT)
    return LayoutType::INLINE;
  // if it's a tag and it's children are of block type it gets block
  else if (m_node->getType() == ItemType::TAG &&
           checkBlockTag(m_node->m_children)) {
    return LayoutType::BLOCK;
    // if the node has children of if it's and input tag
  } else if (!m_node->m_children.empty() || m_node->m_text == "input")
    return LayoutType::INLINE;
  // web developer's mistake for having an empty block type tag!!
  else
    return LayoutType::BLOCK;
}

void BlockLayout::layout(LayoutContext &ctx) {
  m_start_x = m_parent->m_start_x;
  m_width = m_parent->m_width;
  if (m_previous) {
    m_start_y = m_previous->m_start_y + m_previous->m_height;
  } else {
    m_start_y = m_parent->m_start_y;
  }
  LayoutType mode = layout_mode();
  if (mode == LayoutType::BLOCK) {
    Layout *previous = nullptr;
    for (auto &m_child : m_node->m_children) {
      m_children.emplace_back(
          std::make_unique<BlockLayout>(m_child.get(), this, previous));
      previous = m_children.back().get();
    }
  } else {
    new_line();
    recurse(m_node, ctx);
  }

  for (auto &child : m_children) {
    child->layout(ctx);
  }

  for (auto &child : m_children) {
    m_height += child->m_height;
  }
}

void BlockLayout::word(Item *node, LayoutContext &ctx) {
  std::vector<std::string> codepoints = UTF8::word_seperator(node->m_text);
  for (auto &str : codepoints) {
    make_word(node, std::move(str), ctx);
  }
}

void BlockLayout::input(Item *node, LayoutContext &ctx) {
  float w = INPUT_WIDTH_PX;
  if (m_cursor_x + w > m_width)
    new_line();
  auto *layout = m_children.back().get();
  if (layout->getType() != LayoutType::LINE)
    throw LayoutException(
        "Error at BlockLayout::make_word(). Type is not LineLayout.");
  auto *line = static_cast<LineLayout *>(layout);
  TextLayout *previous_word{nullptr};
  if (!line->m_children.empty()) {
    //--NOTE: Only TextLayout are gonna be LineLayout's children so no need to
    //        check
    previous_word = static_cast<TextLayout *>(line->m_children.back().get());
  }
  auto new_input = std::make_unique<InputLayout>(node, line, previous_word);
  line->m_children.push_back(std::move(new_input));

  auto &weight = m_node->m_style["font-weight"];
  auto &size = m_node->m_style["font-size"];
  auto font = ctx.fontCache->get_font(FontCache::get_weight(weight), size);
  float fWidth = FontCache::getUTF8Width(font, " ");
  m_cursor_x += w + fWidth;
  return;
}

void BlockLayout::recurse(Item *root, LayoutContext &ctx) {
  if (root->getType() == ItemType::TEXT) {
    word(root, ctx);
  } else {
    if (root->m_text == "br")
      new_line();
    else if (root->m_text == "input" || root->m_text == "button")
      input(root, ctx);
    else {
      for (auto &child : root->m_children) {
        recurse(child.get(), ctx);
      }
    }
  }
}

//--INFO: relative x postion of the text gets set in word/make_word.
//        absolute x and y position gets set at flush()
void BlockLayout::make_word(Item *node, std::string word, LayoutContext &ctx) {

  // getting formatting info from current Block Node
  auto &weight = node->m_style["font-weight"];
  //--WARNING: Only have one style for nw, stick w/ it. So we ignore this
  [[maybe_unused]] auto &style = node->m_style["font-style"];
  auto &size = node->m_style["font-size"];

  auto font = ctx.fontCache->get_font(FontCache::get_weight(weight), size);

  // Getting the word's dimensions
  float width = FontCache::getUTF8Width(font, word);

  // Getting the space's dimensions
  float Swidth = FontCache::getUTF8Width(font, " ");
  // If cursor moves past this block's width change line
  if (m_cursor_x + width > m_width) {
    new_line();
  }
  m_cursor_x += width + Swidth;

  /*Grabbing the ongoing line layout, creating a new txt layout and adding it to
   * the line*/
  auto *layout = m_children.back().get();
  if (layout->getType() != LayoutType::LINE)
    throw LayoutException(
        "Error at BlockLayout::make_word(). Type is not LineLayout.");
  auto *line = static_cast<LineLayout *>(layout);
  TextLayout *previous_word{nullptr};
  if (!line->m_children.empty()) {
    //--NOTE: Only TextLayout are gonna be LineLayout's children so no need to
    //        check
    previous_word = static_cast<TextLayout *>(line->m_children.back().get());
  }
  auto new_word =
      std::make_unique<TextLayout>(node, std::move(word), line, previous_word);
  line->m_children.push_back(std::move(new_word));
  return;
}

std::vector<std::unique_ptr<DrawItem>> BlockLayout::paint() {
  std::vector<std::unique_ptr<DrawItem>> cmds{};
  std::string bg_color = "transparent";
  if (m_node->m_style.contains("background-color")) {
    float radius = (m_node->m_style.contains("border-radius"))
                       ? std::stof(m_node->m_style["border-radius"].substr(
                             0, m_node->m_style["border-radius"].size() - 2))
                       : 0.0f;
    bg_color = m_node->m_style["background-color"];
    cmds.emplace_back(
        std::make_unique<DrawRRect>(self_rect(), radius, bg_color));
  }
  return cmds;
}

void DocumentLayout::pprint() {
  std::cout << "NOTHING" << std::endl;
  m_children[0]->pprint();
}

void BlockLayout::pprint() {
  if (layout_mode() == LayoutType::BLOCK) {
    std::cout << "BLOCK " << m_node->m_text << std::endl;
    for (auto &node : m_children) {
      static_cast<BlockLayout *>(node.get())->pprint();
    }
  } else {
    for (auto &child : m_children) {
      child.get()->pprint();
    }
  }
}

void BlockLayout::new_line() {
  m_cursor_x = 0;
  LineLayout *lastLine{nullptr};
  if (!m_children.empty()) {
    lastLine = static_cast<LineLayout *>(m_children.back().get());
  }
  auto new_line = std::make_unique<LineLayout>(m_node, this, lastLine);
  m_children.push_back(std::move(new_line));
}

SkRect BlockLayout::self_rect() {
  return SkRect{m_start_x, m_start_y, m_start_x + m_width,
                m_start_y + m_height};
}

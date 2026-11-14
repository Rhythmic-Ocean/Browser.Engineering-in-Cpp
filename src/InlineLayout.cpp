#include "helpers.hpp"
#include "layout.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <include/core/SkFontMetrics.h>
#include <iostream>

using namespace Layout;

void LineLayout::layout(LayoutContext &ctx) {
  m_width = m_parent->m_width;
  m_start_x = m_parent->m_start_x;
  if (m_previous) {
    m_start_y = m_previous->m_start_y + m_previous->m_height;
  } else {
    m_start_y = m_parent->m_start_y;
  }
  for (auto &word : m_children) {
    word->layout(ctx);
  }
  float max_ascent{}, max_descent{};
  getExtremes(max_ascent, max_descent);
  float baseline = m_start_y + 1.25f * static_cast<float>(max_ascent);
  // this assumes all children of LineLayout are text
  for (auto &word : m_children) {
    auto metrics = static_cast<TextLayout *>(word.get())->fontMetrics;
    float font_ascent = std::abs(metrics.fAscent);
    word->m_start_y = (baseline - font_ascent);
  }
  m_height = 1.25f * (max_ascent + max_descent);
}

std::vector<std::unique_ptr<DrawItem>>
LineLayout::paint_effects(std::vector<std::unique_ptr<DrawItem>> cmds) {
  return paint_visual_effect(m_node, std::move(cmds), self_rect());
}

std::vector<std::unique_ptr<DrawItem>> LineLayout::paint() { return {}; }

void LineLayout::getExtremes(float &max_ascent, float &max_descent) {
  for (auto &word : m_children) {
    auto font = static_cast<TextLayout *>(word.get())->fontMetrics;
    max_ascent = std::max(max_ascent, std::abs(font.fAscent));
    max_descent =
        std::max(max_descent, static_cast<float>(std::abs(font.fDescent)));
  }
}

void LineLayout::pprint() {
  std::cout << "NOTHING" << std::endl;
  m_children[0]->pprint();
}

void TextLayout::layout(LayoutContext &ctx) {
  // getting formatting info from current Block Node
  auto &weight = m_node->m_style["font-weight"];
  //--WARNING: Only have one style for nw, stick w/ it. So we ignore this
  [[maybe_unused]] auto &style = m_node->m_style["font-style"];
  auto &size = m_node->m_style["font-size"];

  font = ctx.fontCache->get_font(FontCache::get_weight(weight), size);
  // Getting the word's dimensions
  m_width = FontCache::getUTF8Width(font, m_text);
  // if a previous word exists in the same line
  if (m_previous) {
    float SWidth = FontCache::getUTF8Width(font, " ");
    m_start_x = m_previous->m_start_x + SWidth + m_previous->m_width;
  } else {
    m_start_x = m_parent->m_start_x;
  }
  m_height = font.getMetrics(&fontMetrics);
}

std::vector<std::unique_ptr<DrawItem>>
TextLayout::paint_effects(std::vector<std::unique_ptr<DrawItem>> cmds) {
  return paint_visual_effect(m_node, std::move(cmds), self_rect());
}

std::vector<std::unique_ptr<DrawItem>> TextLayout::paint() {
  auto color = m_node->m_style["color"];
  std::vector<std::unique_ptr<DrawItem>> cmd{};
  auto text =
      std::make_unique<DrawText>(m_text, color, font, m_start_x, m_start_y);
  cmd.push_back(std::move(text));
  return cmd;
}

void TextLayout::pprint() { std::cout << m_text << std::endl; }

void InputLayout::layout(LayoutContext &ctx) {
  // Getting all the styles for input/button text from Browser.css
  auto &weight = m_node->m_style["font-weight"];
  auto &style = m_node->m_style["font-style"];
  float size;
  try {
    size = std::stof(m_node->m_style["font-size"]);
  } catch (std::exception &exception) {
    size = 16.0f;
  }
  font = ctx.fontCache->get_font(FontCache::get_weight(weight), size, style);
  m_width = INPUT_WIDTH_PX;
  assert(m_node->getType() == ItemType::TAG &&
         "Couldn't assert Item's type as Tag at InputLayout::layout()");
  m_text = "";

  float Swidth;
  if (m_previous) {
    Swidth = FontCache::getUTF8Width(font, " ");
    m_start_x = m_previous->m_start_x + Swidth + m_previous->m_width;
  } else {
    m_start_x = m_parent->m_start_x;
  }
  m_height = font.getMetrics(nullptr);
}

std::vector<std::unique_ptr<DrawItem>>
InputLayout::paint_effects(std::vector<std::unique_ptr<DrawItem>> cmds) {
  return paint_visual_effect(m_node, std::move(cmds), self_rect());
}

std::vector<std::unique_ptr<DrawItem>> InputLayout::paint() {
  std::vector<std::unique_ptr<DrawItem>> cmds{};
  // Input's tag's input field rectangle
  std::string bg_color = "transparent";
  std::string color = m_node->m_style["color"];
  if (m_node->m_style.contains("background-color")) {
    bg_color = m_node->m_style["background-color"];
    cmds.emplace_back(std::make_unique<DrawRect>(self_rect(), bg_color));
  }
  Tag *tag = static_cast<Tag *>(m_node);
  if (tag->m_text == "input") {
    if (tag->m_attributes.contains("value")) {
      m_text = tag->m_attributes["value"];
    }
  } else if (tag->m_text == "button") {
    if (tag->m_children.size() == 1 &&
        tag->m_children[0]->getType() == ItemType::TEXT) {
      m_text = tag->m_children[0]->m_text;
    } else {
      std::cerr << "Ignoring html stuff inside button cuz there's more than "
                   "one (just text) right nw"
                << std::endl;
      m_text = "";
    }
  }

  float fWidth =
      font.measureText(m_text.c_str(), m_text.size(), SkTextEncoding::kUTF8);
  m_displayText = m_text;

  // Reseat m_text if text size is too big to fit in the default input box
  if (fWidth != 0 && fWidth > INPUT_WIDTH_PX) {
    float shrink_by = INPUT_WIDTH_PX / fWidth;
    size_t display_text_num = static_cast<size_t>(
        std::round(static_cast<float>(m_text.size()) * shrink_by));
    m_displayText = m_text.substr(m_text.size() - display_text_num);
  }

  auto txt = std::make_unique<DrawText>(m_displayText, color, font, m_start_x,
                                        m_start_y);
  cmds.push_back(std::move(txt));
  if (m_node->focus) {
    float cx = m_start_x + FontCache::getUTF8Width(font, m_displayText);
    cmds.push_back(std::make_unique<DrawLine>(
        cx, m_start_y, cx, m_start_y + m_height, "black", 1));
  }
  return cmds;
}

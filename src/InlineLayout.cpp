#include "helpers.hpp"
#include "layout.hpp"
#include <cassert>
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
    auto *font = static_cast<TextLayout *>(word.get())->font;
    float font_ascent = static_cast<float>(TTF_GetFontAscent(font));
    word->m_start_y = (baseline - font_ascent);
  }
  m_height = 1.25f * (max_ascent + max_descent);
}

std::vector<std::unique_ptr<DrawItem>> LineLayout::paint() { return {}; }

void LineLayout::getExtremes(float &max_ascent, float &max_descent) {
  for (auto &word : m_children) {
    TTF_Font *font = static_cast<TextLayout *>(word.get())->font;
    max_ascent =
        std::max(max_ascent, static_cast<float>(TTF_GetFontAscent(font)));
    max_descent = std::max(
        max_descent, static_cast<float>(std::abs(TTF_GetFontDescent(font))));
  }
}

void LineLayout::pprint() {
  std::cout << "NOTHING" << std::endl;
  m_children[0]->pprint();
}

void TextLayout::layout(LayoutContext &ctx) {
  // getting formatting info from current Block Node
  auto icolor = m_node->m_style["color"];
  auto &weight = m_node->m_style["font-weight"];
  //--WARNING: Only have one style for nw, stick w/ it. So we ignore this
  [[maybe_unused]] auto &style = m_node->m_style["font-style"];
  auto &size = m_node->m_style["font-size"];

  // creating the TTF_Text and coloring it
  auto color = parse_color(icolor);
  font = ctx.fontCache->get_font(FontCache::get_weight(weight),
                                 FontCache::get_size(size));
  auto *txt{
      TTF_CreateText(ctx.textEngine, font, m_word.c_str(), m_word.size())};
  auto *space{TTF_CreateText(ctx.textEngine, font, " ", 1)};
  if (!txt || !space) {
    SDL_Log("Couldn't create text: %s. Error: %s\n", m_word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't create text: " + m_word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  TTF_SetTextColor(txt, color.r, color.g, color.b, color.a);

  // Getting the word's dimensions
  int iHeight{};
  int iWidth{};

  if (!TTF_GetTextSize(txt, &iWidth, &iHeight)) {
    SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", m_word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't calculate string size of: " + m_word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  m_width = static_cast<float>(iWidth);
  // if a previous word exists in the same line
  if (m_previous) {

    if (!TTF_GetTextSize(space, &iWidth, &iHeight)) {
      SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", " ",
              SDL_GetError());
      throw WindowException(
          "Couldn't calculate string size of: " + std::string("space") +
          ". Error: " + std::string(SDL_GetError()));
    }
    float Swidth = static_cast<float>(iWidth);

    m_start_x = m_previous->m_start_x + Swidth + m_previous->m_width;
  } else {
    m_start_x = m_parent->m_start_x;
  }
  m_height = static_cast<float>(TTF_GetFontLineSkip(font));
  m_text.reset(txt);
}

std::vector<std::unique_ptr<DrawItem>> TextLayout::paint() {
  std::vector<std::unique_ptr<DrawItem>> cmd{};
  auto text = std::make_unique<DrawText>(m_text.get(), m_start_x, m_start_y);
  cmd.push_back(std::move(text));
  return cmd;
}

void TextLayout::pprint() { std::cout << m_word << std::endl; }

void InputLayout::layout(LayoutContext &ctx) {
  // Getting all the styles for input/button text from Browser.css
  auto icolor = m_node->m_style["color"];
  auto &weight = m_node->m_style["font-weight"];
  //--WARNING: Only have one style for nw, stick w/ it. So we ignore this
  [[maybe_unused]] auto &style = m_node->m_style["font-style"];
  auto &size = m_node->m_style["font-size"];
  // creating the TTF_Text and coloring it
  auto color = parse_color(icolor);
  font = ctx.fontCache->get_font(FontCache::get_weight(weight),
                                 FontCache::get_size(size));
  m_width = INPUT_WIDTH_PX;
  assert(m_node->getType() == ItemType::TAG &&
         "Couldn't assert Item's type as Tag at InputLayout::layout()");
  m_text = std::unique_ptr<TTF_Text, TextDeleter>(
      TTF_CreateText(ctx.textEngine, font, "", 0));
  if (!m_text.get()) {
    SDL_Log("Couldn't create text: %s. Error: %s\n", m_word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't create text: " + m_word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  TTF_SetTextColor(m_text.get(), color.r, color.g, color.b, color.a);

  if (m_previous) {
    int iWidth;
    if (!TTF_GetStringSize(font, " ", 1, &iWidth, nullptr)) {
      SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", " ",
              SDL_GetError());
      throw WindowException(
          "Couldn't calculate string size of: " + std::string("space") +
          ". Error: " + std::string(SDL_GetError()));
    }
    float Swidth = static_cast<float>(iWidth);
    m_start_x = m_previous->m_start_x + Swidth + m_previous->m_width;
  } else {
    m_start_x = m_parent->m_start_x;
  }
  m_height = static_cast<float>(TTF_GetFontLineSkip(font));
}

std::vector<std::unique_ptr<DrawItem>> InputLayout::paint() {
  std::vector<std::unique_ptr<DrawItem>> cmds{};
  // Input's tag's input field rectangle
  std::string bg_color = "transparent";
  std::string color = m_node->m_style["color"];
  if (m_node->m_style.contains("background-color")) {
    bg_color = m_node->m_style["background-color"];
    cmds.emplace_back(
        std::make_unique<DrawRect>(self_rect(), parse_color(bg_color)));
  }
  // Getting the underlying text inside the InputLayout
  std::string text{};
  Tag *tag = static_cast<Tag *>(m_node);
  if (tag->m_text == "input") {
    if (tag->m_attributes.contains("value")) {
      text = tag->m_attributes["value"];
      std::cerr << text;
    }
  } else if (tag->m_text == "button") {
    if (tag->m_children.size() == 1 &&
        tag->m_children[0]->getType() == ItemType::TEXT) {
      text = tag->m_children[0]->m_text;
    } else {
      std::cerr << "Ignoring html stuff inside button cuz there's more than "
                   "one (just text) right nw"
                << std::endl;
      text = "";
    }
  }
  TTF_SetTextString(m_text.get(), text.c_str(), text.size());
  auto txt = std::make_unique<DrawText>(m_text.get(), m_start_x, m_start_y);
  cmds.push_back(std::move(txt));
  if (m_node->focus) {
    int iWidth;
    TTF_GetTextSize(m_text.get(), &iWidth, nullptr);
    float cx = m_start_x + static_cast<float>(iWidth);
    cmds.push_back(std::make_unique<DrawLine>(
        cx, m_start_y, cx, m_start_y + m_height, parse_color("black"), 1));
  }
  return cmds;
}

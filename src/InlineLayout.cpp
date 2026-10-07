#include "layout.hpp"
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

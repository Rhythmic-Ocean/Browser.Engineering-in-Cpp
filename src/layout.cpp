#pragma once

#include "layout.hpp"
#include "Browser.hpp"
#include "helpers.hpp"
#include "window.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>

using namespace Layout;

/*--NOTE: DrawItem's member function definitions
          Each item is rendered individually here
*/

void DrawText::execute(float scroll_y, SDL_Renderer *renderer) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  float cur_scroll_y{m_top - scroll_y};
  if (!TTF_DrawRendererText(m_text, m_left, cur_scroll_y)) {
    SDL_Log("Failed to draw text at item %s: %s\n", m_text->text,
            SDL_GetError());
  }
}

void DrawRect::execute(float scroll_y, SDL_Renderer *renderer) {
  float cur_scroll_y{rect.y - scroll_y};
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  if (!SDL_RenderFillRect(renderer, &rect)) {
    SDL_Log("Failed to render layout rectangle: %s\n", SDL_GetError());
  }
}

//--NOTE: Layout's member function definitions

void DocumentLayout::layout(TTF_TextEngine *textEngine) {
  BlockLayout *child = new BlockLayout(m_node);
  m_children.emplace_back(child);
  m_width = Browser::WIDTH - 2 * HSTEP;
  m_start_x = HSTEP;
  m_start_y = VSTEP;
  child->layout(textEngine);
  m_height = child->m_height;
}

std::vector<DisplayItem> DocumentLayout::paint() { return {}; }

void getExtremes(int &max_ascent, int &max_descent, int &max_lineskip,
                 std::vector<DisplayItem *> &line);
Layout::Layout(Item *rootNode) : m_rootNode{rootNode} {}
void Layout::init(TTF_TextEngine *textEngine) {
  m_textEngine = textEngine;
  m_fontCache.init();
}

void Layout::open_tag(const std::string &tag) {
  using enum FontStyle;
  if (tag == "i") {
    if (g_fontStyle == BOLD)
      g_fontStyle = BOLD_ITALICS;
    else
      g_fontStyle = ITALICS;
  } else if (tag == "b") {
    if (g_fontStyle == ITALICS)
      g_fontStyle = BOLD_ITALICS;
    else
      g_fontStyle = BOLD;
  } else if (tag == "small")
    g_fontSize = 10;
  else if (tag == "big")
    g_fontSize += 20;
  else if (tag == "br")
    m_breakLine = true;
}

void Layout::close_tag(const std::string &tag) {
  using enum FontStyle;
  if (tag == "i") {
    if (g_fontStyle == BOLD_ITALICS)
      g_fontStyle = BOLD;
    else
      g_fontStyle = REGULAR;
  } else if (tag == "b") {
    if (g_fontStyle == BOLD_ITALICS)
      g_fontStyle = ITALICS;
    else
      g_fontStyle = REGULAR;
  } else if (tag == "small")
    g_fontSize += 10;
  else if (tag == "big")
    g_fontSize = 20;
  else if (tag == "p") {
    m_breakLine = true;
    m_lineSpacing = VSTEP;
  }
}

/*
--WARNING: The multi-byte parsing algorithm in the following functioin was
           generated with heavy AI assistance. Extensive line-by-line notes for
           explanative purpose.
 */
void Layout::process_text(const std::string &str) {
  std::string word{};
  for (size_t c = 0; c < str.size(); ++c) {
    unsigned char byte = static_cast<unsigned char>(str[c]);
    /*--NOTE: Collapsing consecutive whitespaces into a single space.
              Important to note that the whitespaces we see in webpages are
              the result of tags, not excess ones from plain text in a .html
              file!!*/
    if (std::isspace(byte)) {
      if (!word.empty()) {
        m_items.push_back(std::move(make_display(word)));
      }
      while (c < str.size() &&
             std::isspace(static_cast<unsigned char>(str[c]))) {
        ++c;
      }
      --c;
      std::string space_str = " ";
      m_items.push_back(std::move(make_display(space_str)));
      continue;
    }
    /*--NOTE: To support multi-byte unicode characters. Multi-byte characters
              stores how many bytes it's made up of in it's first byte as
              follows:
              - 0xxxxxxx -> 1 byte chars has first bit set 0, rest of 7 bits
                store info.
                That's why we & the byte with 0x10000000, and if it gives 0x00
                it's a one byte char
              - 11xxxxxx -> this first byte pattern is for 2 byte chars
              - 111xxxxx -> this first byte pattern is for 3 byte chars
              - 1111xxxx -> this first byte pattern is for 4 byte chars
     */
    if ((byte & 0x80) == 0x00) { // & with 0x10000000
      word += str[c];
    } else {
      if (!word.empty()) { // if the char is no 1 byte then we immediatly
                           // clear the buffer and start a new item
        m_items.push_back(std::move(make_display(word)));
      }
      size_t char_length = 2;
      if ((byte & 0xF0) == 0xE0) // & w/ 0x11110000
        char_length = 3;
      else if ((byte & 0xF8) == 0xF0) //& w/ 0x11111000
        char_length = 4;
      std::string utf8_char = str.substr(c, char_length);
      // each of the multi byte char are treated as seperate display item
      m_items.push_back(std::move(make_display(utf8_char)));
      c += (char_length - 1); // as the enclosing for loop performs ++c next
    }
  }
  // Flush the remaining chars from buffer
  if (!word.empty())
    m_items.push_back(std::move(make_display(word)));
}

void Layout::recurse(Item *root) {
  std::string word{};
  if (root->getType() == ItemType::TEXT) {
    process_text(root->m_text);
  } else {
    open_tag(root->m_text);
    for (auto &child : root->m_children) {
      recurse(child.get());
    }
    close_tag(root->m_text);
  }
}

DisplayItem Layout::make_display(std::string &word) {
  int h{};
  int w{};
  DisplayItem item{};
  auto *font = m_fontCache.get_font(g_fontStyle, g_fontSize);
  auto *txt{TTF_CreateText(m_textEngine, font, word.c_str(), word.size())};

  if (!txt) {
    SDL_Log("Couldn't create text: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't create text: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  TTF_SetTextColor(txt, 255, 255, 255, 255);
  item.text_obj.reset(txt);
  if (!TTF_GetTextSize(txt, &w, &h)) {
    SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't calculate string size of: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  item.width = static_cast<float>(w);
  item.height = static_cast<float>(h);
  item.font = font;
  item.internal->breakLine = m_breakLine;
  item.internal->lineSpace = m_lineSpacing;
  m_breakLine = false;
  m_lineSpacing = 0;
  word.clear();
  return item;
}

void Layout::calculate_position(SDL_Renderer &renderer) {
  if (m_items.empty())
    return;
  size_t total_items{m_items.size()};
  int current_w, current_h;
  SDL_GetCurrentRenderOutputSize(&renderer, &current_w, &current_h);
  auto current_line_top{DEFAULT_MARGIN};
  auto end_x{current_w - DEFAULT_MARGIN};
  int i{};
  while (i < total_items) {
    std::vector<DisplayItem *> current_line{};
    float line_width = DEFAULT_MARGIN;
    while (i < total_items) {
      float item_w = m_items[i].width;
      if ((!current_line.empty() && (line_width + item_w > end_x)) ||
          (!current_line.empty() && m_items[i].internal->breakLine))
        break;
      line_width += item_w;
      current_line.push_back(&m_items[i]);
      ++i;
    }
    int max_ascent{}, max_descent{}, max_lineskip{};
    getExtremes(max_ascent, max_descent, max_lineskip, current_line);
    float baseline_y = current_line_top + max_ascent;
    float pen_x = DEFAULT_MARGIN;
    for (auto *word : current_line) {
      word->x = pen_x;
      TTF_Font *font = word->font;
      float font_ascent = TTF_GetFontAscent(font);
      word->y = baseline_y - font_ascent;
      pen_x += word->width;
    }
    current_line_top += std::max(max_ascent + max_descent, max_lineskip);
    current_line_top += (i >= total_items) ? 0 : m_items[i].internal->lineSpace;
  }
  Window::max_y = current_line_top;
}

void getExtremes(int &max_ascent, int &max_descent, int &max_lineskip,
                 std::vector<DisplayItem *> &line) {
  for (auto *word : line) {
    TTF_Font *font = word->font;
    max_ascent = std::max(max_ascent, TTF_GetFontAscent(font));
    max_descent = std::max(max_descent, std::abs(TTF_GetFontDescent(font)));
    max_lineskip = std::max(max_lineskip, TTF_GetFontLineSkip(font));
  }
}

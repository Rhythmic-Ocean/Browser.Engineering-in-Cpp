#include "layout.hpp"
#include "helpers.hpp"
#include "window.hpp"
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>

void getExtremes(int &max_ascent, int &max_descent, int &max_lineskip,
                 std::vector<DisplayItem *> &line);
Layout::Layout() {}
void Layout::init(TTF_TextEngine *textEngine) {
  m_textEngine = textEngine;
  m_fontCache.init();
}

void Layout::lex(const std::string &body) {
  std::vector<std::unique_ptr<Item>> out{};
  std::string word{};
  bool in_tag = false;
  for (size_t c = 0; c < body.size(); ++c) {
    if (body[c] == '<') {
      in_tag = true;
      out.emplace_back(new Text{word}); // pushinig in actual content
      word.clear();
    } else if (body[c] == '>') {
      in_tag = false;
      out.emplace_back(new Tag{word}); // pushing in html stuffs
      word.clear();
    } else if (!in_tag && body[c] == '&' && body.size() - c >= 4 &&
               body.compare(c, 4, "&lt;") == 0) {
      word += "<";
      c += 3; // skip past "lt;" (loop's ++c handles the last +1)
    } else if (!in_tag && body[c] == '&' && body.size() - c >= 4 &&
               body.compare(c, 4, "&gt;") == 0) {
      word += '>';
      c += 3;
    } else
      word += body[c];
  }
  if (!word.empty())
    out.emplace_back(new Text{word});
  process_layout(out);
}

void Layout::process_layout(const std::vector<std::unique_ptr<Item>> &tokens) {
  std::vector<DisplayItem> items{};
  std::string word{};

  for (const auto &token : tokens) {
    if (token->getType() == ItemType::TAG) {
      if (!word.empty()) {
        items.push_back(std::move(make_display(word)));
      }
      set_font(token->m_text);
    } else {
      const auto &str = token->m_text;
      for (size_t c = 0; c < str.size(); ++c) {
        unsigned char byte = static_cast<unsigned char>(str[c]);

        if (std::isspace(byte)) {
          if (!word.empty()) {
            items.push_back(std::move(make_display(word)));
          }

          while (c < str.size() &&
                 std::isspace(static_cast<unsigned char>(str[c]))) {
            ++c;
          }
          --c;

          std::string space_str = " ";
          items.push_back(std::move(make_display(space_str)));
          continue;
        }

        if ((byte & 0x80) == 0x00) {
          word += str[c];
        } else {
          if (!word.empty()) {
            items.push_back(std::move(make_display(word)));
          }

          size_t char_length = 2;
          if ((byte & 0xF0) == 0xE0)
            char_length = 3;
          else if ((byte & 0xF8) == 0xF0)
            char_length = 4;

          std::string utf8_char = str.substr(c, char_length);
          items.push_back(std::move(make_display(utf8_char)));
          c += (char_length - 1);
        }
      }

      if (!word.empty()) {
        items.push_back(std::move(make_display(word)));
      }
    }
  }

  m_items = std::move(items);
}

void Layout::set_font(const std::string &fontTag) {
  using enum FontStyle;
  FontStyle oldStyle = g_fontStyle;
  if (fontTag == "i") {
    g_fontStyle = ITALICS;
  } else if (fontTag == "b") {
    g_fontStyle = BOLD;
  } else if (fontTag == "/b" || fontTag == "/i") {
    g_fontStyle = REGULAR;
  } else if (fontTag == "small")
    g_fontSize = 10;
  else if (fontTag == "/small")
    g_fontSize += 10;
  else if (fontTag == "big")
    g_fontSize += 20;
  else if (fontTag == "/big")
    g_fontSize = 20;
  else if (fontTag == "/p") {
    m_breakLine = true;
    m_lineSpacing = VSTEP;
  } else if (fontTag == "br")
    m_breakLine = true;

  if ((oldStyle == BOLD && g_fontStyle == ITALICS) ||
      (oldStyle == ITALICS && g_fontStyle == BOLD)) {
    g_fontStyle = BOLD_ITALICS;
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

#pragma once

#include "fonts.hpp"
#include "helpers.hpp"
#include <SDL3_ttf/SDL_textengine.h>
#include <vector>

class Layout {
  static constexpr float DEFAULT_MARGIN = 20.0f;
  TTF_TextEngine *m_textEngine{nullptr};
  FontCache m_fontCache{};

  FontSize g_fontSize = BASE_FONT_SIZE;
  FontStyle g_fontStyle = FontStyle::REGULAR;

  void process_layout(const std::vector<Item> &tokens);
  DisplayItem make_display(std::string &str);
  void set_font(const std::string &fontTag);

public:
  std::vector<DisplayItem> m_items{};
  Layout();
  void init(TTF_TextEngine *textEngine);
  void lex(const std::string &body);
  void calculate_position(SDL_Renderer &renderer);
};

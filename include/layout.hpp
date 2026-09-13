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
  FontStyle g_fontStyle = FontStyle::REGULAR; // prob make a vector later on cuz
                                              // we can stack a lottt of styles

  void process_text(const std::string &text);
  void open_tag(const std::string &tag);
  void close_tag(const std::string &tag);
  DisplayItem make_display(std::string &str);
  bool m_breakLine{false};
  int m_lineSpacing{};

public:
  void recurse(Item *root);
  std::vector<DisplayItem> m_items{};
  Layout();
  void init(TTF_TextEngine *textEngine);
  void calculate_position(SDL_Renderer &renderer);
};

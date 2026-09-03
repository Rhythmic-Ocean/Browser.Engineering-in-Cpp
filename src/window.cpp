#include "window.hpp"
#include "helpers.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

Window::Window(const std::string &title, int width, int height)
    : m_title{title}, m_width{width}, m_height{height} {
  init();
  load_media();
  load_engine();
}

void Window::init() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
    throw WindowException("SDL Initialization failed: " +
                          std::string(SDL_GetError()));
  }
  if (!TTF_Init()) {
    SDL_Log("TTF could not initiate! TTF error: %s\n", SDL_GetError());
    throw WindowException("TTF Initialization failed: " +
                          std::string(SDL_GetError()));
  }

  SDL_Window *raw_window{};
  SDL_Renderer *raw_renderer{};
  if (!SDL_CreateWindowAndRenderer(m_title.c_str(), m_width, m_height,
                                   SDL_WINDOW_RESIZABLE, &raw_window,
                                   &raw_renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed!!: %s]n", SDL_GetError());
    throw WindowException("SDL_CreateWindowAndRenderer failed!!" +
                          std::string(SDL_GetError()));
  }
  m_window.reset(raw_window);
  m_renderer.reset(raw_renderer);
}
void Window::load_media() {
  const std::string fontPath{"../assets/fonts/OpenSans-Regular.ttf"};
  const std::string fallbackPath{"../assets/fonts/NotoSansSC-Regular.ttf"};
  const std::string fontPath3{"../assets/fonts/OpenSans-Italic.ttf"};
  const std::string fontPath4{"../assets/fonts/OpenSans-Bold.ttf"};
  const std::string fontPath5{"../assets/fonts/OpenSans-BoldItalic.ttf"};

  auto load_font_with_fallback = [&](const std::string &path) -> TTF_Font * {
    TTF_Font *font = TTF_OpenFont(path.c_str(), 16.0f);
    if (!font) {
      return nullptr;
    }
    TTF_Font *fallback = TTF_OpenFont(fallbackPath.c_str(), 16.0f);
    if (!fallback) {
      TTF_CloseFont(font);
      return nullptr;
    }
    if (!TTF_AddFallbackFont(font, fallback)) {
      TTF_CloseFont(fallback);
      TTF_CloseFont(font);
      return nullptr;
    }
    return font;
  };

  m_font.reset(load_font_with_fallback(fontPath));
  m_italics.reset(load_font_with_fallback(fontPath3));
  m_bold.reset(load_font_with_fallback(fontPath4));
  m_boldItalics.reset(load_font_with_fallback(fontPath5));

  if (!m_font || !m_italics || !m_bold || !m_boldItalics) {
    SDL_Log("TTF_OpenFont Error: %s", SDL_GetError());
    throw WindowException("TTF_OpenFont Error: " + std::string(SDL_GetError()));
  }
}

void Window::load_engine() {
  TTF_TextEngine *raw_engine{TTF_CreateRendererTextEngine(m_renderer.get())};
  if (!raw_engine) {
    SDL_Log("Couldn't create text engine: %s\n", SDL_GetError());
    throw WindowException(
        "Couldn't create text engine: " + std::string(SDL_GetError()) + "\n");
  }
  m_engine.reset(raw_engine);
}

void Window::start_event() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      is_Running = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      m_width = event.window.data1;
      m_height = event.window.data2;
      calculate_position(*this);
      break;
    case SDL_EVENT_MOUSE_WHEEL: {
      scroll_y -= event.wheel.y * 40.0f;
      if (scroll_y < 0.0f)
        scroll_y = 0.0f;
      float max_scroll = max_y - m_height;
      if (max_scroll < 0.0f)
        max_scroll = 0.0f;
      if (scroll_y > max_scroll) {
        scroll_y = max_scroll;
      }
      break;
    }
    default:
      break;
    }
  }
}

TTF_Font *Window::choose_font() {
  switch (style) {
  case BOLD:
    return m_bold.get();
  case ITALICS:
    return m_italics.get();
  case BOLD_ITALICS:
    return m_boldItalics.get();
  default:
    return m_font.get();
  }
}

DisplayItem Window::make_display(std::string &word) {
  int h{};
  int w{};
  DisplayItem item{};
  auto *font = choose_font();
  auto *txt{TTF_CreateText(m_engine.get(), font, word.c_str(), word.size())};

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
  item.width = w;
  item.height = h;
  word.clear();
  return item;
}

void Window::lex(const std::string &body) {
  std::vector<Item> out{};
  std::string word{};
  bool in_tag = false;
  for (size_t c = 0; c < body.size(); ++c) {
    if (body[c] == '<') {
      in_tag = true;
      out.push_back({word, false}); // pushinig in actual content
      word.clear();
    } else if (body[c] == '>') {
      in_tag = false;
      out.push_back({word, true}); // pushing in html stuffs
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
    out.push_back({word, false});
  process_layout(out);
}

void Window::get_font(std::string &str) {
  if (str == "i") {
    if (style == BOLD) {
      style = BOLD_ITALICS;
    } else
      style = ITALICS;
  } else if (str == "b") {
    if (style == ITALICS) {
      style = BOLD_ITALICS;
    } else
      style = BOLD;
  } else
    style = REGULAR;
  return;
}

void Window::process_layout(std::vector<Item> &tokens) {
  std::vector<DisplayItem> items{};
  std::string word{};
  for (auto &token : tokens) {
    if (token.m_tag) {
      get_font(token.m_text);
    } else {
      auto &str = token.m_text;
      for (int c{}; c < str.size(); ++c) {
        if (std::isspace(str[c])) {
          if (!word.empty()) {
            items.push_back(std::move(make_display(word)));
            word.clear();
          }
          word += ' ';
          items.push_back(std::move(make_display(word)));
          continue;
        }
        unsigned char first_byte{static_cast<unsigned char>(str[c])};
        if ((first_byte & 0x80) == 0x00) {
          word += str[c];
        } else {
          if (!word.empty()) {
            items.push_back(std::move(make_display(word)));
            word.clear();
          }
          int char_length = 2;
          if ((first_byte & 0xF0) == 0xE0)
            char_length = 3;
          else if ((first_byte & 0xF8) == 0xF0)
            char_length = 4;
          std::string cjk_char = str.substr(c, char_length);
          items.push_back(std::move(make_display(cjk_char)));
          c += (char_length - 1);
        }
      }
      if (!word.empty()) {
        items.push_back(std::move(make_display(word)));
      }
    }
  }
  m_items = std::move(items);
  calculate_position(*this);
}

void calculate_position(Window &window) {
  int current_w, current_h;
  SDL_GetCurrentRenderOutputSize(window.m_renderer.get(), &current_w,
                                 &current_h);
  auto end_x{current_w - Window::DEFAULT_MARGIN};
  auto cursor_x{Window::DEFAULT_MARGIN};
  auto cursor_y{Window::DEFAULT_MARGIN};
  int line_skip{TTF_GetFontLineSkip(window.m_font.get())};
  for (int i{}; i < window.m_items.size(); ++i) {
    if ((cursor_x + window.m_items[i].width) > end_x) {
      cursor_x = Window::DEFAULT_MARGIN;
      cursor_y += line_skip;
    }
    window.m_items[i].x = cursor_x;
    window.m_items[i].y = cursor_y;
    cursor_x += window.m_items[i].width;
  }
  window.max_y = cursor_y + line_skip;
}

void Window::draw_text() {
  SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(m_renderer.get());
  for (int i{}; i < m_items.size(); ++i) {
    float cur_scroll_y{m_items[i].y - scroll_y};
    if (cur_scroll_y + m_items[i].height < 0.0f)
      continue;
    if (cur_scroll_y > m_height)
      break;
    if (!TTF_DrawRendererText(m_items[i].text_obj.get(), m_items[i].x,
                              cur_scroll_y)) {
      SDL_Log("Failed to draw text at item %d: %s\n", i, SDL_GetError());
    }
  }
  SDL_RenderPresent(m_renderer.get());
}

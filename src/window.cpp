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

  std::string fontPath{"../assets/fonts/OpenSans-Regular.ttf"};
  std::string fontPath2{"../assets/fonts/NotoSansSC-Regular.ttf"};
  TTF_Font *raw_font{TTF_OpenFont(fontPath.c_str(), 16.0f)};
  TTF_Font *raw_font2{TTF_OpenFont(fontPath2.c_str(), 16.0f)};
  if (!raw_font || !raw_font2) {
    SDL_Log("TTF_OpenFont Error: %s", SDL_GetError());
    throw WindowException("TTF_OpenFont Error: " + std::string(SDL_GetError()));
  }
  TTF_AddFallbackFont(raw_font, raw_font2);
  m_font.reset(raw_font);

  TTF_TextEngine *raw_engine{TTF_CreateRendererTextEngine(raw_renderer)};
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

SDL_Texture *Window::create_texture(std::string &load) {
  SDL_Color textColour{255, 255, 255};
  std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
      TTF_RenderText_Blended_Wrapped(m_font.get(), load.c_str(), load.size(),
                                     textColour, 760),
      &SDL_DestroySurface};
  if (!surface.get()) {
    SDL_Log("TTF_RenderText_Blended_Wrapped Error: %s", SDL_GetError());
    throw WindowException("TTF_RenderText_Blended_Wrapped Error: " +
                          std::string(SDL_GetError()));
  }
  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{
      SDL_CreateTextureFromSurface(m_renderer.get(), surface.get()),
      &SDL_DestroyTexture};
  if (!texture.get()) {
    SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
    throw WindowException("SDL_CreateTextureFromSurface Error: " +
                          std::string(SDL_GetError()));
  }
  return texture.release();
}

void Window::clear() { SDL_RenderClear(m_renderer.get()); }
void Window::presentTexture(SDL_Texture *texture) {
  float texW = 0.0f, texH = 0.0f;
  SDL_GetTextureSize(texture, &texW, &texH);

  SDL_FRect dstRect = {.x = (800.0f - texW) / 2.0f, // Centered
                       .y = 20.0f,
                       .w = texW,
                       .h = texH};
  SDL_SetRenderDrawColor(m_renderer.get(), 24, 24, 30, 255);
  SDL_RenderClear(m_renderer.get());
  SDL_RenderTexture(m_renderer.get(), texture, nullptr, &dstRect);
  SDL_RenderPresent(m_renderer.get());
}

DisplayItem Window::make_display(std::string &word) {
  int h{};
  int w{};
  DisplayItem item{};
  auto *txt{
      TTF_CreateText(m_engine.get(), m_font.get(), word.c_str(), word.size())};
  if (!txt) {
    SDL_Log("Couldn't create text: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't create text: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
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
  std::vector<DisplayItem> items{};
  std::string word{};
  int char_length{1};
  int count_char{};
  bool in_tag = false;
  bool in_char = false;
  for (size_t c = 0; c < body.size(); ++c) {
    if (body[c] == '<') {
      in_tag = true;
    } else if (body[c] == '>') {
      in_tag = false;
    } else if (!in_tag && body[c] == '&' && body.size() - c >= 4 &&
               body.compare(c, 4, "&lt;") == 0) {
      word += "<";
      items.push_back(std::move(make_display(word)));
      c += 3; // skip past "lt;" (loop's ++c handles the last +1)
    } else if (!in_tag && body[c] == '&' && body.size() - c >= 4 &&
               body.compare(c, 4, "&gt;") == 0) {
      word += '>';
      items.push_back(std::move(make_display(word)));
      c += 3;
    } else if (!in_tag) {
      if (std::isspace(body[c])) {
        if (!word.empty()) {
          items.push_back(std::move(make_display(word)));
          word.clear();
        }
        std::string space_str = " ";
        items.push_back(std::move(make_display(space_str)));
        continue;
      }

      unsigned char first_byte = static_cast<unsigned char>(body[c]);
      if ((first_byte & 0x80) == 0x00) {
        word += body[c]; // Add to the word, but DON'T push it yet!
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
        std::string cjk_char = body.substr(c, char_length);
        items.push_back(std::move(make_display(cjk_char)));
        c += (char_length - 1);
      }
    }
  }
  if (!word.empty())
    items.push_back(std::move(make_display(word)));
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
    TTF_DrawRendererText(m_items[i].text_obj.get(), m_items[i].x, cur_scroll_y);
  }
  SDL_RenderPresent(m_renderer.get());
}

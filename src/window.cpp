#include "window.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include <SDL3/SDL_error.h>
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
  load_engine();
  layout.init(m_engine.get());
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
  SDL_Window *raw_window = nullptr;
  SDL_Renderer *raw_renderer = nullptr;

  if (!SDL_CreateWindowAndRenderer(m_title.c_str(), m_width, m_height,
                                   SDL_WINDOW_RESIZABLE, &raw_window,
                                   &raw_renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    throw WindowException("SDL_CreateWindowAndRenderer failed: " +
                          std::string(SDL_GetError()));
  }

  m_window.reset(raw_window);
  m_renderer.reset(raw_renderer);
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
      layout.calculate_position(*m_renderer.get());
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

void Window::draw_text(std::vector<DisplayItem> &items) {
  SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(m_renderer.get());
  for (int i{}; i < items.size(); ++i) {
    float cur_scroll_y{items[i].y - scroll_y};
    if (cur_scroll_y + items[i].height < 0.0f)
      continue;
    if (cur_scroll_y > m_height)
      break;
    if (!TTF_DrawRendererText(items[i].text_obj.get(), items[i].x,
                              cur_scroll_y)) {
      SDL_Log("Failed to draw text at item %d: %s\n", i, SDL_GetError());
    }
  }
  SDL_RenderPresent(m_renderer.get());
}

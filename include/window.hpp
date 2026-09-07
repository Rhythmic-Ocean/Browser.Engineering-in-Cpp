#pragma once

#include "helpers.hpp"
#include "layout.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

class Window {
  std::string m_title;
  int m_width;
  int m_height;
  std::unique_ptr<SDL_Window, WindowDeleter> m_window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> m_renderer;
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine{};
  float scroll_y{0.0f};
  float max_y{0.0f};

  void init();
  void load_engine();

public:
  Layout layout{};
  bool is_Running{true};
  Window(const std::string &title, int width, int height);
  SDL_Renderer *getRenderer() const { return m_renderer.get(); }
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&other) = delete;
  Window &operator=(Window &&other) = delete;
  ~Window() = default;

  void start_event();
  void draw_text(std::vector<DisplayItem> &items);
};

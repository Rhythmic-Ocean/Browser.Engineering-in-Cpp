#pragma once

#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <SDL3_ttf/SDL_textengine.h>
#include <memory>

// browser own the nodes too
class Browser {
public:
private:
  std::string m_title;
  Layout::LayoutContext ctx{};
  std::unique_ptr<SDL_Window, WindowDeleter> m_window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> m_renderer;
  std::unique_ptr<Item> m_rootNode{};
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine{};
  std::unique_ptr<Layout::Layout> m_document{};

  inline static float m_scroll_y;
  inline static float m_max_y;
  bool is_Running{true};
  std::vector<std::unique_ptr<Layout::DrawItem>> m_displayItems{};

  void init();
  void load_engine();
  void start_event();
  void paint_tree(Layout::Layout *layoutNode);
  void draw();

public:
  inline static float m_width = WIDTH;
  inline static float m_height = HEIGHT;
  Browser(std::string &&title, float width = WIDTH, float height = HEIGHT)
      : m_title{std::move(title)} {
    m_width = width;
    m_height = height;
    init();
    load_engine();
  };
  void load(URL &url);
};

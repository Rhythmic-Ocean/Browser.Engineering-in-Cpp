#pragma once

#include "Parser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <memory>

struct Size {
  int width;
  int height;
};

class Browser {
public:
private:
  std::string m_title;
  Layout::LayoutContext ctx{};
  std::unique_ptr<SDL_Window, WindowDeleter> m_window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> m_renderer;
  std::unique_ptr<Item> m_rootNode{};
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine{};
  std::unique_ptr<Layout::FontCache> m_fontCache{};
  std::unique_ptr<Layout::Layout> m_document{};
  URL m_url{};

  inline static float m_scroll_y;
  inline static float m_max_y;
  std::vector<std::unique_ptr<Layout::DrawItem>> m_displayItems{};

  void init();
  void load_engine();
  void paint_tree(Layout::Layout *layoutNode);
  void style(Item *node, Parser::StyleSheet &rules);
  void click(SDL_MouseButtonEvent &event);
  std::vector<std::string_view> get_links(const std::vector<Item *> &list);
  void ctx_setup();

public:
  bool is_Running{true};
  void draw();
  void start_event();
  int m_width{};
  int m_height{};
  Browser(std::string &&title, Size size = {WIDTH, HEIGHT})
      : m_title{std::move(title)}, m_width{size.width}, m_height{size.height} {
    init();
    load_engine();
    ctx_setup();
  };
  void load(URL url);
};

#pragma once

#include "Parser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

namespace browser {
struct Size {
  int width;
  int height;
};

struct TabContext {
  Layout::LayoutContext lctx{};
  TTF_TextEngine *textEngine{};
  Layout::FontCache *fontCache{};
  Size size;
};

class Tab {
public:
private:
  Layout::LayoutContext ctx{};
  std::unique_ptr<Item> m_rootNode{};
  TTF_TextEngine *m_engine{};
  Layout::FontCache *m_fontCache{};
  std::unique_ptr<Layout::Layout> m_document{};
  URL m_url{};

  inline static float m_scroll_y;
  inline static float m_max_y;
  std::vector<std::unique_ptr<Layout::DrawItem>> m_displayItems{};

  void paint_tree(Layout::Layout *layoutNode);
  void style(Item *node, Parser::StyleSheet &rules);
  std::vector<std::string_view> get_links(const std::vector<Item *> &list);

public:
  void click(float x, float y);
  void scroll(float turn);
  void draw(SDL_Renderer *renderer);
  int m_width{};
  int m_height{};
  void load(URL url);
  Tab(TabContext tctx);
  ~Tab() = default;
};

class Browser {
  std::vector<std::unique_ptr<Tab>> tabs{};
  Tab *active_tab{};

  std::string m_title;
  Layout::LayoutContext ctx{};
  std::unique_ptr<SDL_Window, WindowDeleter> m_window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> m_renderer;
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine{};
  std::unique_ptr<Layout::FontCache> m_fontCache{};

  void init();
  void load_engine();
  void ctx_setup();

public:
  void draw();
  void start_event();
  void new_tab(URL url);
  bool is_Running;
  int m_width{};
  int m_height{};
  Browser(std::string &&title, Size size = {WIDTH, HEIGHT})
      : m_title{std::move(title)}, m_width{size.width}, m_height{size.height} {
    init();
    load_engine();
    ctx_setup();
  }
};

} // namespace browser

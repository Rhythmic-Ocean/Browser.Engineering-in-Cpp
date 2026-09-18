#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include "window.hpp"
#include <SDL3_ttf/SDL_textengine.h>
#include <memory>

// browser own the nodes too
class Browser {
public:
  static struct LayoutContext {
    TTF_TextEngine *textEngine;
  } ctx;

private:
  static constexpr float WIDTH = 800.0f;
  static constexpr float HEIGHT = 600.0f;
  std::string m_title;
  int m_width = WIDTH;
  int m_height = HEIGHT;
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

public:
  Browser(std::string &title) : m_title{title} {
    init();
    load_engine();
  };
  void load(URL &url);

private:
public:
};

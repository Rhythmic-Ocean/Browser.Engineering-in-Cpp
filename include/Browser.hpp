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

enum class FOCUS { ADDRESS_BAR, CONTENT, CHROME, NONE };
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
  Parser::StyleSheet m_rules{};
  std::unique_ptr<Item> m_rootNode{};
  TTF_TextEngine *m_engine{};
  Layout::FontCache *m_fontCache{};
  std::unique_ptr<Layout::Layout> m_document{};
  float tab_height{};
  Item *m_focus{nullptr};

  inline static float m_scroll_y;
  inline static float m_max_y;
  std::vector<std::unique_ptr<Layout::DrawItem>> m_displayItems{};

  void paint_tree(Layout::Layout *layoutNode);
  void style(Item *node, Parser::StyleSheet &rules);
  std::vector<std::string_view> get_links(const std::vector<Item *> &list);
  void scrolldown();
  void render();

public:
  void go_back();
  std::vector<URL> m_history{};
  void click(float x, float y, SDL_Window *window);
  void scroll(float turn);
  void draw(SDL_Renderer *renderer, float offset);
  int m_width{};
  int m_height{};
  void load(URL url);
  Tab(TabContext tctx, float l_tab_height);
  ~Tab() = default;
  void handle_special_keys(SDL_Keycode key);
  void handle_input(std::string str);
  void blur() {
    if (m_focus != nullptr) {
      m_focus->focus = false;
      m_focus = nullptr;
      render();
    }
  }
};

class Browser;
class Chrome {
  Browser *m_browser;
  TTF_Font *m_font{};
  float m_fontHeight{};
  float m_padding{};
  float m_tabbar_top{};
  float m_tabbar_bottom{};
  float m_urlbar_top{};
  float m_urlbar_bottom{};
  Layout::Rect m_newTab_rect{};
  Layout::Rect m_back_rect{};
  Layout::Rect m_address_rect{};
  SDL_Rect m_input_rect{};
  // actual address bar string
  // TTF_Version that would be set to reflect m_address_str right before
  // rendering
  std::unique_ptr<TTF_Text, Layout::TextDeleter> m_address_bar{};
  std::unique_ptr<TTF_Text, Layout::TextDeleter> m_plus;
  std::unique_ptr<TTF_Text, Layout::TextDeleter> m_back;
  std::vector<std::unique_ptr<TTF_Text, Layout::TextDeleter>> m_labelNames{};

  Layout::Rect tab_rect(size_t i);

public:
  FOCUS m_focus{FOCUS::NONE};
  std::string m_address_str{};
  float m_bottom;
  void click(float x, float y);
  Chrome(Browser *l_browser);
  std::vector<std::unique_ptr<Layout::DrawItem>> paint();
  void create_labelText();
  void handle_special_keys(SDL_Keycode key);
  void blur() { m_focus = FOCUS::NONE; }
};

class Browser {
public:
  std::vector<std::unique_ptr<Tab>> tabs{};
  Tab *active_tab{};
  std::unique_ptr<TTF_Text, Layout::TextDeleter> url_name{};
  std::string m_title;
  Layout::LayoutContext ctx{};
  std::unique_ptr<SDL_Window, WindowDeleter> m_window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> m_renderer;
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine{};
  std::unique_ptr<Layout::FontCache> m_fontCache{};
  bool is_Running;
  int m_width{};
  int m_height{};
  void handle_special_keys(SDL_Keycode key);
  void handle_input(const SDL_Event &event);
  FOCUS m_focus{FOCUS::NONE};

private:
  std::unique_ptr<Chrome> m_chrome{nullptr};

  void init();
  void load_engine();
  void ctx_setup();
  void click(SDL_MouseButtonEvent event);

public:
  void draw();
  void start_event();
  void new_tab(URL url);
  Browser(std::string &&title, Size size = {WIDTH, HEIGHT})
      : m_title{std::move(title)}, m_width{size.width}, m_height{size.height} {
    init();
    load_engine();
    ctx_setup();
    m_chrome = std::make_unique<Chrome>(this);
  }
};

} // namespace browser

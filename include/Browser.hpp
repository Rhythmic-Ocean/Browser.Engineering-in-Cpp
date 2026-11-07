#pragma once

#include "Parser.hpp"
#include "helpers.hpp"
#include "jsContext.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <include/core/SkAlphaType.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkSurface.h>
#include <memory>
#include <optional>

namespace browser {

enum class FOCUS { ADDRESS_BAR, CONTENT, CHROME, NONE };
struct Size {
  int width;
  int height;
};

struct TabContext {
  Layout::LayoutContext lctx{};
  Layout::FontCache *fontCache{};
  Size size;
};

class Tab {
public:
  std::unique_ptr<Item> m_rootNode{};
  std::vector<std::string> allowed_origins{};
  std::unique_ptr<Layout::Layout> m_document{};
  inline static float m_scroll_y;

private:
  Layout::FontCache *m_fontCache{};
  Layout::LayoutContext ctx{};
  Parser::StyleSheet m_rules{};
  float tab_height{};
  Item *m_focus{nullptr};
  std::optional<jsContext::JSContext> m_js{};

  inline static float m_max_y;
  std::vector<std::unique_ptr<Layout::DrawItem>> m_displayItems{};

  void paint_tree(Layout::Layout *layoutNode);
  void style(Item *node, Parser::StyleSheet &rules);
  std::vector<std::string_view> get_css_links(const std::vector<Item *> &list);
  std::vector<std::string_view> get_js_links(const std::vector<Item *> &list);
  void scrolldown();
  void submit_form(Tag *tag);

public:
  void render();
  void go_back();
  std::vector<url::URL> m_history{};
  void click(float x, float y, SDL_Window *window);
  void scroll(float turn);
  void raster(SkCanvas *canvas);
  int m_width{};
  int m_height{};
  void load(url::URL url, std::string payload = "");
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
  bool allowed_request(const url::URL &l_url);
};

class Browser;
class Chrome {
  Browser *m_browser;
  SkFont m_font{};
  SkFontMetrics m_fontMetrics{};
  float m_fontHeight{};
  float m_tabbar_top{};
  float m_tabbar_bottom{};
  float m_urlbar_top{};
  float m_urlbar_bottom{};
  SkRect m_newTab_rect{};
  SkRect m_back_rect{};
  SkRect m_address_rect{};
  SkRect m_input_rect{};
  std::string text_color = "black";
  // actual address bar string
  // TTF_Version that would be set to reflect m_address_str right before
  // rendering
  std::string m_plus;
  std::string m_back;
  std::vector<std::string> m_labelNames{};

  SkRect tab_rect(size_t i);

public:
  float m_padding{};
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
  std::unique_ptr<Layout::FontCache> m_fontCache{};
  std::vector<std::unique_ptr<Tab>> tabs{};
  Tab *active_tab{};
  std::string m_title;
  Layout::LayoutContext ctx{};
  std::unique_ptr<SDL_Window, WindowDeleter> m_window;
  bool is_Running;
  int m_width{};
  int m_height{};
  void handle_special_keys(SDL_Keycode key);
  void handle_input(const SDL_Event &event);
  FOCUS m_focus{FOCUS::NONE};
  void raster_tab();
  void raster_chrome();

private:
  std::unique_ptr<Chrome> m_chrome{nullptr};
  sk_sp<SkSurface> rootSurface;
  sk_sp<SkSurface> m_chromeSurface;
  sk_sp<SkSurface> m_tabSurface;
  void init();
  void ctx_setup();
  void click(SDL_MouseButtonEvent event);

public:
  void draw();
  void start_event();
  void new_tab(url::URL url);
  Browser(std::string &&title, Size size = {WIDTH, HEIGHT})
      : m_title{std::move(title)}, m_width{size.width}, m_height{size.height} {
    init();
    ctx_setup();
    m_chrome = std::make_unique<Chrome>(this);
    m_chromeSurface = SkSurfaces::Raster(SkImageInfo::Make(
        m_width, static_cast<int>(ceil(m_chrome->m_bottom)),
        SkColorType::kRGBA_8888_SkColorType, SkAlphaType::kPremul_SkAlphaType));
    m_tabSurface = nullptr;
  }
};

} // namespace browser

#include "Browser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

using namespace browser;
void Browser::init() {
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

  if (!SDL_CreateWindowAndRenderer(m_title.c_str(), m_width, m_height, 0,
                                   &raw_window, &raw_renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    throw WindowException("SDL_CreateWindowAndRenderer failed: " +
                          std::string(SDL_GetError()));
  }

  if (!SDL_SetRenderVSync(raw_renderer, 1)) {
    SDL_Log("SDL_SetRenderVSync failed: %s\n", SDL_GetError());
    throw WindowException("SDL_SetRenderVSync failed: " +
                          std::string(SDL_GetError()));
  }

  m_window.reset(raw_window);
  m_renderer.reset(raw_renderer);
  is_Running = true;
}

void Browser::load_engine() {
  TTF_TextEngine *raw_engine{TTF_CreateRendererTextEngine(m_renderer.get())};
  if (!raw_engine) {
    SDL_Log("Couldn't create text engine: %s\n", SDL_GetError());
    throw WindowException(
        "Couldn't create text engine: " + std::string(SDL_GetError()) + "\n");
  }
  m_engine.reset(raw_engine);
}

void Browser::ctx_setup() {
  m_fontCache = std::make_unique<Layout::FontCache>();
  m_fontCache->init();
  ctx.textEngine = m_engine.get();
  ctx.fontCache = m_fontCache.get();
  ctx.windowHeight = static_cast<float>(m_height);
  ctx.windowWidth = static_cast<float>(m_width);
}

void Browser::start_event() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      is_Running = false;
      break;
    case SDL_EVENT_MOUSE_WHEEL: {
      active_tab->scroll(event.wheel.y);
      draw();
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
      click(event.button);
      break;
    }
    default:
      break;
    }
  }
}

void Browser::draw() {
  SDL_SetRenderDrawColor(m_renderer.get(), 255, 255, 255, 255);
  SDL_RenderClear(m_renderer.get());
  active_tab->draw(m_renderer.get(), m_chrome->m_bottom);
  auto list = m_chrome->paint();
  for (size_t i{}; i < list.size(); ++i) {
    list[i]->execute(0, m_renderer.get());
  }
  SDL_RenderPresent(m_renderer.get());
}

void Browser::new_tab(URL url) {
  TabContext tctx{};
  tctx.lctx = ctx;
  tctx.textEngine = m_engine.get();
  tctx.fontCache = m_fontCache.get();
  tctx.size = {m_height, m_width};
  auto new_tab = std::make_unique<Tab>(
      std::move(tctx), static_cast<float>(m_height) - m_chrome->m_bottom);
  new_tab->load(std::move(url));
  active_tab = new_tab.get();
  tabs.push_back(std::move(new_tab));
  m_chrome->create_labelText();
  draw();
}

void Browser::click(SDL_MouseButtonEvent event) {
  if (event.y < m_chrome->m_bottom)
    m_chrome->click(event.x, event.y);
  else {
    float tab_y = event.y - m_chrome->m_bottom;
    active_tab->click(event.x, tab_y);
  }
  draw();
}

Chrome::Chrome(Browser *l_browser) : m_browser{l_browser} {
  auto *fontCache = l_browser->m_fontCache.get();
  m_font = fontCache->get_font(Layout::FontWeight::REGULAR, 20);
  m_fontHeight = static_cast<float>(TTF_GetFontLineSkip(m_font));
  m_padding = 5;
  m_tabbar_top = 0;
  m_tabbar_bottom = m_fontHeight + 2 * m_padding;
  m_urlbar_top = m_tabbar_bottom;
  m_urlbar_bottom = m_urlbar_top + m_fontHeight + 2 * m_padding;
  m_bottom = m_urlbar_bottom;
  int iWidth;
  TTF_GetStringSize(m_font, "+", 1, &iWidth, nullptr);
  float plus_width = static_cast<float>(iWidth) + 2 * m_padding;
  TTF_GetStringSize(m_font, "<", 1, &iWidth, nullptr);
  float back_width = static_cast<float>(iWidth) + 2 * m_padding;
  m_newTab_rect = Layout::Rect{m_padding, m_padding, m_padding + plus_width,
                               m_padding + m_fontHeight};
  m_back_rect =
      Layout::Rect{m_padding, m_urlbar_top + m_padding, m_padding + back_width,
                   m_urlbar_bottom - m_padding};
  m_address_rect =
      Layout::Rect{m_back_rect.right + m_padding, m_urlbar_top + m_padding,
                   WIDTH - m_padding, m_urlbar_bottom - m_padding};
  auto raw_plus = TTF_CreateText(m_browser->m_engine.get(), m_font, "+", 1);
  auto raw_back = TTF_CreateText(m_browser->m_engine.get(), m_font, "<", 1);
  SDL_Color black = Layout::parse_color("black");
  TTF_SetTextColor(raw_plus, black.r, black.g, black.b, black.a);
  TTF_SetTextColor(raw_back, black.r, black.g, black.b, black.a);
  m_plus.reset(raw_plus);
  m_back.reset(raw_back);
}

Layout::Rect Chrome::tab_rect(size_t i) {
  float tab_start = m_newTab_rect.right + m_padding;
  int iWidth;
  TTF_GetStringSize(m_font, "Tab X", 5, &iWidth, nullptr);
  float tab_width = static_cast<float>(iWidth) + 2 * m_padding;
  return Layout::Rect{
      tab_start + tab_width * static_cast<float>(i), m_tabbar_top,
      tab_start + tab_width * static_cast<float>(i + 1), m_tabbar_bottom};
}

std::vector<std::unique_ptr<Layout::DrawItem>> Chrome::paint() {
  std::vector<std::unique_ptr<Layout::DrawItem>> cmds{};
  cmds.emplace_back(std::make_unique<Layout::DrawRect>(
      Layout::Rect{0, 0, static_cast<float>(m_browser->m_width), m_bottom},
      Layout::parse_color("white")));
  cmds.push_back(std::make_unique<Layout::DrawLine>(
      0, m_bottom, static_cast<float>(m_browser->m_width), m_bottom,
      Layout::parse_color("black"), 1));
  auto outline = Layout::DrawOutline::createOutline(
      m_newTab_rect, Layout::parse_color("white"), Layout::parse_color("black"),
      1);
  auto newText = std::make_unique<Layout::DrawText>(
      m_plus.get(), m_newTab_rect.left + m_padding, m_newTab_rect.top);
  cmds.push_back(std::move(outline));
  cmds.push_back(std::move(newText));
  for (size_t i{}; i < m_labelNames.size(); ++i) {
    auto &tab = m_browser->tabs[i];
    auto bounds = tab_rect(i);
    auto leftLine = std::make_unique<Layout::DrawLine>(
        bounds.left, 0, bounds.left, bounds.bottom,
        Layout::parse_color("black"), 1);
    auto rightLine = std::make_unique<Layout::DrawLine>(
        bounds.right, 0, bounds.right, bounds.bottom,
        Layout::parse_color("black"), 1);
    cmds.push_back(std::move(leftLine));
    cmds.push_back(std::move(rightLine));
    cmds.push_back(std::make_unique<Layout::DrawText>(m_labelNames[i].get(),
                                                      bounds.left + m_padding,
                                                      bounds.top + m_padding));

    if (tab.get() == m_browser->active_tab) {
      cmds.push_back(std::make_unique<Layout::DrawLine>(
          0, bounds.bottom, bounds.left, bounds.bottom,
          Layout::parse_color("red"), 1));
      cmds.push_back(std::make_unique<Layout::DrawLine>(
          bounds.right, bounds.bottom, static_cast<float>(m_browser->m_width),
          bounds.bottom, Layout::parse_color("blue"), 1));
    }

    // Back Button
    cmds.push_back(Layout::DrawOutline::createOutline(
        m_back_rect, Layout::parse_color("white"), Layout::parse_color("black"),
        1));
    cmds.push_back(std::make_unique<Layout::DrawText>(
        m_back.get(), m_back_rect.left + m_padding, m_back_rect.top));

    // Address Bar
    auto url =
        TTF_CreateText(m_browser->m_engine.get(), m_font,
                       m_browser->active_tab->m_history.back().to_str().c_str(),
                       m_browser->active_tab->m_history.back().to_str().size());
    auto black = Layout::parse_color("black");
    TTF_SetTextColor(url, black.r, black.g, black.b, black.a);
    cmds.push_back(std::make_unique<Layout::DrawText>(
        url, m_address_rect.left + m_padding, m_address_rect.top));
  }

  return cmds;
}

void Chrome::click(float x, float y) {
  // clicking the '+' sign
  if (m_newTab_rect.contains_point(x, y)) {
    m_browser->new_tab(URL("https://browser.engineering/"));
  } else if (m_back_rect.contains_point(x, y)) {
    m_browser->active_tab->go_back();
  } else {
    for (size_t i{}; i < m_browser->tabs.size(); ++i) {
      if (tab_rect(i).contains_point(x, y)) {
        m_browser->active_tab = m_browser->tabs[i].get();
      }
    }
  }
}

void Chrome::create_labelText() {
  // if number of labels and tabs are the same just return;
  if (m_browser->tabs.size() == m_labelNames.size())
    return;
  // if number of actual tabs are less than labels, i.e some have been closed,
  // we pop the labels
  while (m_browser->tabs.size() < m_labelNames.size()) {
    m_labelNames.pop_back();
  }
  // if a new tab's opened
  while (m_browser->tabs.size() > m_labelNames.size()) {
    std::string tab_name = "Tab " + std::to_string(m_browser->tabs.size() - 1);
    auto text = TTF_CreateText(m_browser->m_engine.get(), m_font,
                               tab_name.c_str(), tab_name.size());
    SDL_Color black = Layout::parse_color("black");
    TTF_SetTextColor(text, black.r, black.g, black.b, black.a);
    m_labelNames.emplace_back(
        std::unique_ptr<TTF_Text, Layout::TextDeleter>(text));
  }
}

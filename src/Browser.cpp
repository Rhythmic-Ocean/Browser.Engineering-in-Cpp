#include "Browser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <include/core/SkAlphaType.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorType.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkPixmap.h>
#include <include/core/SkRect.h>
#include <include/core/SkSurface.h>
#include <memory>

using namespace browser;
void Browser::init() {

  rootSurface = SkSurfaces::Raster(
      SkImageInfo::Make(m_width, m_height, SkColorType::kRGBA_8888_SkColorType,
                        SkAlphaType::kUnpremul_SkAlphaType));

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
  SDL_Window *raw_window =
      SDL_CreateWindow(m_title.c_str(), m_width, m_height, 0);

  if (!raw_window) {
    SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    throw WindowException("SDL_CreateWindow failed: " +
                          std::string(SDL_GetError()));
  }
  m_window.reset(raw_window);
  is_Running = true;
}

void Browser::ctx_setup() {
  m_fontCache = std::make_unique<Layout::FontCache>();
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
    // Handling escape, backspace and escape keys!
    case (SDL_EVENT_KEY_DOWN): {
      handle_special_keys(event.key.key);
      break;
    }
    case (SDL_EVENT_TEXT_INPUT): {
      handle_input(event);
      break;
    }
    default:
      break;
    }
  }
}

void Browser::draw() {
  SkCanvas *canvas = rootSurface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  SkRect tab_rect =
      SkRect::MakeLTRB(0, m_chrome->m_bottom, static_cast<float>(m_width),
                       static_cast<float>(m_height));
  float tab_offset = m_chrome->m_bottom - active_tab->m_scroll_y;
  canvas->save();
  canvas->clipRect(tab_rect);
  canvas->translate(0, tab_offset);
  m_tabSurface->draw(canvas, 0, 0);
  canvas->restore();

  SkRect chrome_rect =
      SkRect::MakeLTRB(0, 0, static_cast<float>(m_width), m_chrome->m_bottom);
  canvas->save();
  canvas->clipRect(chrome_rect);
  m_chromeSurface->draw(canvas, 0, 0);
  canvas->restore();

  SkPixmap pixMap;
  auto skia_image = rootSurface->makeImageSnapshot();
  skia_image->peekPixels(&pixMap);
  SDL_Surface *sdl_surface =
      SDL_CreateSurfaceFrom(m_width, m_height, SDL_PIXELFORMAT_RGBA32,
                            pixMap.writable_addr(), 4 * m_width);
  SDL_Rect rect = SDL_Rect(0, 0, m_width, m_height);
  auto window_surface = SDL_GetWindowSurface(m_window.get());
  SDL_BlitSurface(sdl_surface, &rect, window_surface, &rect);
  SDL_DestroySurface(sdl_surface);
  SDL_UpdateWindowSurface(m_window.get());
}

void Browser::new_tab(url::URL url) {
  TabContext tctx{};
  tctx.lctx = ctx;
  tctx.fontCache = m_fontCache.get();
  tctx.size = {m_height, m_width};
  auto new_tab = std::make_unique<Tab>(
      std::move(tctx), static_cast<float>(m_height) - m_chrome->m_bottom);
  new_tab->load(std::move(url));
  active_tab = new_tab.get();
  tabs.push_back(std::move(new_tab));
  m_chrome->create_labelText();
  raster_chrome();
  raster_tab();
  draw();
}

void Browser::click(SDL_MouseButtonEvent event) {
  SDL_StopTextInput(m_window.get());
  if (event.y < m_chrome->m_bottom) {
    m_focus = FOCUS::CHROME;
    active_tab->blur();
    m_chrome->click(event.x, event.y);
    raster_chrome();
  } else {
    if (m_focus != FOCUS::CONTENT) {
      m_focus = FOCUS::CONTENT;
      m_chrome->blur();
      raster_chrome();
    }
    std::string url = active_tab->m_history.back().origin();
    float tab_y = event.y - m_chrome->m_bottom;
    active_tab->click(event.x, tab_y, m_window.get());
    if (active_tab->m_history.back().origin() != url) {
      raster_chrome();
    }
    raster_tab();
  }
  draw();
}

void Browser::handle_special_keys(SDL_Keycode key) {
  if (m_focus == FOCUS::CHROME) {
    m_chrome->handle_special_keys(key);
    raster_chrome();

  } else if (m_focus == FOCUS::CONTENT) {
    active_tab->handle_special_keys(key);
    raster_tab();
  } else
    return;
  draw();
}

void Browser::handle_input(const SDL_Event &event) {
  if (m_focus == FOCUS::CHROME && m_chrome->m_focus == FOCUS::ADDRESS_BAR) {
    std::string str = event.text.text;
    for (char c : str) {
      m_chrome->m_address_str.push_back(c);
    }
    raster_chrome();
  } else if (m_focus == FOCUS::CONTENT) {
    std::string str = event.text.text;
    active_tab->handle_input(std::move(str));
    raster_tab();
  }
  draw();
}

void Browser::raster_tab() {
  int tab_height = static_cast<int>(
      ceil(active_tab->m_document->m_height + 2 * Layout::VSTEP));
  if (!m_tabSurface || tab_height != m_tabSurface->height()) {
    m_tabSurface = SkSurfaces::Raster(SkImageInfo::Make(
        m_width, tab_height, SkColorType::kRGBA_8888_SkColorType,
        SkAlphaType::kPremul_SkAlphaType));
  }
  SkCanvas *canvas = m_tabSurface->getCanvas();
  canvas->clear(SK_ColorWHITE);
  active_tab->raster(canvas);
}

void Browser::raster_chrome() {
  SkCanvas *canvas = m_chromeSurface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  auto list = m_chrome->paint();
  for (size_t i{}; i < list.size(); ++i) {
    list[i]->execute(canvas);
  }
}

Chrome::Chrome(Browser *l_browser) : m_browser{l_browser} {
  auto *fontCache = l_browser->m_fontCache.get();
  m_font = fontCache->get_font(Layout::FontCache::get_weight("regular"), 20.0f);
  m_fontHeight = m_font.getMetrics(&m_fontMetrics);
  m_padding = 5;
  m_tabbar_top = 0;
  m_tabbar_bottom = m_fontHeight + 2 * m_padding;
  m_urlbar_top = m_tabbar_bottom;
  m_urlbar_bottom = m_urlbar_top + m_fontHeight + 2 * m_padding;
  m_bottom = m_urlbar_bottom;
  float plus_width = Layout::FontCache::getUTF8Width(m_font, "+");
  float back_width = Layout::FontCache::getUTF8Width(m_font, "<");
  m_newTab_rect = SkRect{m_padding, m_padding, 3 * m_padding + plus_width,
                         m_padding + m_fontHeight};
  m_back_rect = SkRect{m_padding, m_urlbar_top + m_padding,
                       3 * m_padding + back_width, m_urlbar_bottom - m_padding};
  m_address_rect =
      SkRect{m_back_rect.right() + m_padding, m_urlbar_top + m_padding,
             WIDTH - m_padding, m_urlbar_bottom - m_padding};
  m_plus = "+";
  m_back = "<";
  m_focus = FOCUS::NONE;
}

SkRect Chrome::tab_rect(size_t i) {
  float tab_start = m_newTab_rect.right() + m_padding;
  float tab_width =
      Layout::FontCache::getUTF8Width(m_font, "Tab X") + 2 * m_padding;
  return SkRect::MakeLTRB(
      tab_start + tab_width * static_cast<float>(i), m_tabbar_top,
      tab_start + tab_width * static_cast<float>(i + 1), m_tabbar_bottom);
}

std::vector<std::unique_ptr<Layout::DrawItem>> Chrome::paint() {
  std::vector<std::unique_ptr<Layout::DrawItem>> cmds{};
  // Chrome white rect
  cmds.emplace_back(std::make_unique<Layout::DrawRect>(
      SkRect{0, 0, static_cast<float>(m_browser->m_width), m_bottom}, "white"));
  // Chrome's end line
  cmds.push_back(std::make_unique<Layout::DrawLine>(
      0, m_bottom, static_cast<float>(m_browser->m_width), m_bottom, "black",
      1));
  // +'s rectangle' rectangle'
  auto outline =
      std::make_unique<Layout::DrawOutline>(m_newTab_rect, "black", 1);

  // the plus text
  auto newText = std::make_unique<Layout::DrawText>(
      m_plus, text_color, m_font, m_newTab_rect.left() + m_padding,
      m_newTab_rect.top());
  cmds.push_back(std::move(outline));
  cmds.push_back(std::move(newText));
  // Different tabs
  for (size_t i{}; i < m_labelNames.size(); ++i) {
    auto &tab = m_browser->tabs[i];
    auto bounds = tab_rect(i);
    auto leftLine = std::make_unique<Layout::DrawLine>(
        bounds.left(), 0, bounds.left(), bounds.bottom(), "black", 1);
    auto rightLine = std::make_unique<Layout::DrawLine>(
        bounds.right(), 0, bounds.right(), bounds.bottom(), "black", 1);
    cmds.push_back(std::move(leftLine));
    cmds.push_back(std::move(rightLine));
    cmds.push_back(std::make_unique<Layout::DrawText>(
        m_labelNames[i], text_color, m_font, bounds.left() + m_padding,
        bounds.top() + m_padding));

    if (tab.get() == m_browser->active_tab) {
      cmds.push_back(std::make_unique<Layout::DrawLine>(
          0, bounds.bottom(), bounds.left(), bounds.bottom(), "red", 1));
      cmds.push_back(std::make_unique<Layout::DrawLine>(
          bounds.right(), bounds.bottom(),
          static_cast<float>(m_browser->m_width), bounds.bottom(), "blue", 1));
    }
  }

  // Back Button
  cmds.push_back(
      std::make_unique<Layout::DrawOutline>(m_back_rect, "black", 1));
  cmds.push_back(std::make_unique<Layout::DrawText>(
      m_back, text_color, m_font, m_back_rect.left() + m_padding,
      m_back_rect.top()));

  // Address Bar
  cmds.push_back(
      std::make_unique<Layout::DrawOutline>(m_address_rect, "black", 1));
  if (m_focus == FOCUS::ADDRESS_BAR) {
    // the actual address bar text that the user's editing
    cmds.push_back(std::make_unique<Layout::DrawText>(
        m_address_str, text_color, m_font, m_address_rect.left() + m_padding,
        m_address_rect.top()));
    float width = Layout::FontCache::getUTF8Width(m_font, m_address_str);
    // the cursor
    cmds.push_back(std::make_unique<Layout::DrawLine>(
        m_address_rect.left() + m_padding + width, m_address_rect.top(),
        m_address_rect.left() + m_padding + width, m_address_rect.bottom(),
        "black", 1));

  } else {
    cmds.push_back(std::make_unique<Layout::DrawText>(
        m_browser->active_tab->m_history.back().m_url, text_color, m_font,
        m_address_rect.left() + m_padding, m_address_rect.top()));
  }

  return cmds;
}

void Chrome::click(float x, float y) {
  // clicking on the address bar to type url, and as they type we display!
  if (m_address_rect.contains(x, y)) {
    m_focus = FOCUS::ADDRESS_BAR;
    SDL_StartTextInput(m_browser->m_window.get());
    m_address_str = "";
    return;
  }
  // clicking the '+' sign
  if (m_newTab_rect.contains(x, y)) {
    m_browser->new_tab(url::URL("https://browser.engineering/"));
  }
  // clicking on '<' back button
  else if (m_back_rect.contains(x, y)) {
    m_browser->active_tab->go_back();
    m_browser->raster_chrome();
    m_browser->raster_tab();
  }
  // clicking on different tabs for navigation
  else {
    for (size_t i{}; i < m_browser->tabs.size(); ++i) {
      if (tab_rect(i).contains(x, y)) {
        m_browser->active_tab = m_browser->tabs[i].get();
      }
    }
  }
  m_focus = FOCUS::NONE;
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
    m_labelNames.emplace_back(tab_name);
  }
}

void Chrome::handle_special_keys(SDL_Keycode key) {
  if (m_focus != FOCUS::ADDRESS_BAR)
    return;
  switch (key) {
  case SDLK_ESCAPE: {
    m_focus = FOCUS::NONE;
    SDL_StopTextInput(m_browser->m_window.get());
    break;
  }
  case SDLK_BACKSPACE: {
    if (!m_address_str.empty()) {
      m_address_str.pop_back();
    }
    break;
  }
  case SDLK_RETURN: {
    m_focus = FOCUS::NONE;
    SDL_StopTextInput(m_browser->m_window.get());
    url::URL new_url = url::URL(m_address_str);
    m_browser->active_tab->load(std::move(new_url));
    break;
  }
  }
}

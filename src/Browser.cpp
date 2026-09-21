#include "Browser.hpp"
#include "HTMLParse.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <algorithm>

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

  m_window.reset(raw_window);
  m_renderer.reset(raw_renderer);
  m_scroll_y = 0.0f;
  m_max_y = 0.0f;
}

void Browser::start_event() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      is_Running = false;
      break;
    case SDL_EVENT_MOUSE_WHEEL: {
      m_max_y = std::max(m_document->m_height + 2 * VSTEP - m_height, 0.0f);
      m_scroll_y -= event.wheel.y * 40.0f;
      if (m_scroll_y < 0.0f)
        m_scroll_y = 0.0f;
      if (m_scroll_y > m_max_y) {
        m_scroll_y = m_max_y;
      }
      draw();
      break;
    }
    default:
      break;
    }
  }
}

void Browser::paint_tree(Layout::Layout *layoutNode) {
  auto displayVec = layoutNode->paint();
  for (auto *item : displayVec) {
    m_displayItems.emplace_back(item);
  }
  for (auto &child : layoutNode->m_children) {
    paint_tree(child.get());
  }
}

void Browser::load(URL &url) {
  std::string response = url.request();
  HTMLParse parser{response};
  m_rootNode.reset(parser.parse()); // layout has to own the root node...
  m_fontCache = std::make_unique<Layout::FontCache>();
  m_fontCache->init();
  // make layout object indep of window??
  // Browser can own layout and window both...
  ctx.textEngine = m_engine.get();
  ctx.fontCache = m_fontCache.get();
  ctx.windowHeight = m_height;
  ctx.windowWidth = m_width;
  m_document = std::make_unique<Layout::DocumentLayout>(m_rootNode.get());
  m_document->layout(ctx);
  paint_tree(m_document.get());
  while (is_Running) {
    start_event();
    draw();
  }
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

void Browser::draw() {

  SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(m_renderer.get());
  for (auto &cmd : m_displayItems) {
    if (cmd->m_top > m_scroll_y + m_height)
      break; // if u below the screen just stop
    if (cmd->m_bottom < m_scroll_y)
      continue;
    cmd->execute(m_scroll_y, m_renderer.get());
  }
  SDL_RenderPresent(m_renderer.get());
}

#include "Browser.hpp"
#include "HTMLParse.hpp"
#include "layout.hpp"
#include "url.hpp"

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

  if (!SDL_CreateWindowAndRenderer(m_title.c_str(), m_width, m_height,
                                   SDL_WINDOW_RESIZABLE, &raw_window,
                                   &raw_renderer)) {
    SDL_Log("SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    throw WindowException("SDL_CreateWindowAndRenderer failed: " +
                          std::string(SDL_GetError()));
  }

  m_window.reset(raw_window);
  m_renderer.reset(raw_renderer);
  m_scroll_y = 0.0f;
  m_max_y = 0.0f;
}

void Window::start_event() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      is_Running = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      m_width = event.window.data1;
      m_height = event.window.data2;
      layout.calculate_position(*m_renderer.get());
      break;
    case SDL_EVENT_MOUSE_WHEEL: {
      scroll_y -= event.wheel.y * 40.0f;
      if (scroll_y < 0.0f)
        scroll_y = 0.0f;
      float max_scroll = max_y - m_height;
      if (max_scroll < 0.0f)
        max_scroll = 0.0f;
      if (scroll_y > max_scroll) {
        scroll_y = max_scroll;
      }
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
  Window window{"Browser", 800, 600};
  HTMLParse parser{response};
  m_rootNode.reset(parser.parse()); // layout has to own the root node...
  // make layout object indep of window??
  // Browser can own layout and window both...
  ctx.textEngine = m_engine.get();
  m_document = std::make_unique<Layout::DocumentLayout>(m_rootNode.get());
  m_document.layout();
  window.layout.recurse(root);
  window.layout.calculate_position(*window.getRenderer());
  while (window.is_Running) {
    window.start_event();
    window.draw_text(window.layout.m_items);
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

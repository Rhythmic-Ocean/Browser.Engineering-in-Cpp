#include "Browser.hpp"
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
      active_tab->click(event.button.x, event.button.y);
      draw();
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
  active_tab->draw(m_renderer.get());
}

void Browser::new_tab(URL url) {
  TabContext tctx{};
  tctx.lctx = ctx;
  tctx.textEngine = m_engine.get();
  tctx.fontCache = m_fontCache.get();
  tctx.size = {m_height, m_width};
  auto new_tab = std::make_unique<Tab>(std::move(tctx));
  new_tab->load(std::move(url));
  active_tab = new_tab.get();
  tabs.push_back(std::move(new_tab));
  draw();
}

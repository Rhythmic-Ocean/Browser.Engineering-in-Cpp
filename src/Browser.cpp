#include "Browser.hpp"
#include "HTMLParse.hpp"
#include "url.hpp"

void Browser::load(URL &url) {
  std::string response = url.request();
  Window window{"Browser", 800, 600};
  HTMLParse parser{response};
  m_rootNode.reset(parser.parse()); // layout has to own the root node...
  // make layout object indep of window??
  // Browser can own layout and window both...
  m_document = std::make_unique<Layout>(m_rootNode.get());
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

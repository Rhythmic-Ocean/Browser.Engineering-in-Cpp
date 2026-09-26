#include "Browser.hpp"
#include "Parser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <algorithm>
#include <exception>
#include <iterator>

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
  Parser::HTMLParser parser{response};
  m_rootNode.reset(parser.parse()); // layout has to own the root node...

  auto default_css = hlp::get_default_CSS();
  auto rules = Parser::CSSParser(default_css).parse();
  std::vector<std::string_view> css_links =
      get_links(hlp::tree_to_list(m_rootNode.get()));
  for (auto link : css_links) {
    auto style_url = url.resolve(link);
    std::string body{};
    try {
      body = style_url.request();
    } catch (std::exception &exc) {
      continue;
    }
    auto source = Parser::CSSParser(body).parse();
    rules.insert(rules.end(), std::make_move_iterator(source.begin()),
                 std::make_move_iterator(source.end()));
  }

  auto cascade_priority = [](Parser::StyleRule &rule1,
                             Parser::StyleRule &rule2) {
    return rule1.selector->priority < rule2.selector->priority;
  };

  std::ranges::sort(rules, cascade_priority);
  style(m_rootNode.get(), rules);
  m_fontCache = std::make_unique<Layout::FontCache>();
  m_fontCache->init();
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

void Browser::style(Item *node, Parser::StyleSheet &rules) {
  if (node->getType() == ItemType::TAG) {
    Tag *tag = static_cast<Tag *>(node);
  }
  /*--NOTE: The inline style attribute overrides the one in stylesheet, so it
            shall come after!!*/
  for (auto &rules : rules) {
    if (!rules.selector->matches(node))
      continue;
    for (auto &property : rules.property) {
      node->m_style[property.first] = property.second;
    }
  }
  if (node->getType() == ItemType::TAG &&
      static_cast<Tag *>(node)->m_attributes.contains("style")) {
    Tag *tag = static_cast<Tag *>(node);
    auto pairs = Parser::CSSParser(tag->m_attributes["style"]).body();
    for (auto pair : pairs) {
      tag->m_style[pair.first] = pair.second;
    }
  }
  for (auto &child : node->m_children) {
    style(child.get(), rules);
  }
}

std::vector<std::string_view>
Browser::get_links(const std::vector<Item *> &list) {
  std::vector<std::string_view> links{};
  for (auto *node : list) {
    if ((node->getType() != ItemType::TAG) && node->m_text != "link")
      continue;
    auto tag = static_cast<Tag *>(node);
    if (tag->m_attributes["rel"] == "stylesheet" &&
        tag->m_attributes.contains("href"))
      links.push_back(tag->m_attributes["href"]);
  }
  return links;
}

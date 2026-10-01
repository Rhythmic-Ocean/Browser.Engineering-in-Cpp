#include "Browser.hpp"
#include "Parser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <exception>
#include <iterator>
#include <string>

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
      m_max_y =
          std::max(m_document->m_height + 2.0f * static_cast<float>(VSTEP) -
                       static_cast<float>(m_height),
                   0.0f);
      m_scroll_y -= event.wheel.y * 40.0f;
      if (m_scroll_y < 0.0f)
        m_scroll_y = 0.0f;
      if (m_scroll_y > m_max_y) {
        m_scroll_y = m_max_y;
      }
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

void Browser::paint_tree(Layout::Layout *layoutNode) {
  auto displayVec = layoutNode->paint();
  for (auto *item : displayVec) {
    m_displayItems.emplace_back(item);
  }
  for (auto &child : layoutNode->m_children) {
    paint_tree(child.get());
  }
}

void Browser::ctx_setup() {
  m_fontCache = std::make_unique<Layout::FontCache>();
  m_fontCache->init();
  ctx.textEngine = m_engine.get();
  ctx.fontCache = m_fontCache.get();
  ctx.windowHeight = static_cast<float>(m_height);
  ctx.windowWidth = static_cast<float>(m_width);
}

void Browser::load(URL url) {
  m_url = std::move(url);
  std::string response = m_url.request();
  Parser::HTMLParser parser{response};
  m_rootNode.reset(parser.parse()); // layout has to own the root node...

  auto default_css = hlp::get_default_CSS();
  auto rules = Parser::CSSParser(default_css).parse();
  std::vector<std::string_view> css_links =
      get_links(hlp::tree_to_list(m_rootNode.get()));
  for (auto link : css_links) {
    auto style_url = m_url.resolve(link);
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
  m_document = std::make_unique<Layout::DocumentLayout>(m_rootNode.get());
  m_document->layout(ctx);
  m_displayItems.clear();
  paint_tree(m_document.get());
  return;
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

  SDL_SetRenderDrawColor(m_renderer.get(), 255, 255, 255, 255);
  SDL_RenderClear(m_renderer.get());
  for (size_t i{}; i < m_displayItems.size(); ++i) {
    auto &cmd = m_displayItems[i];
    if (cmd->m_top > m_scroll_y + static_cast<float>(m_height))
      break; // if u below the screen just stop
    if (cmd->m_bottom < m_scroll_y)
      continue;
    cmd->execute(m_scroll_y, m_renderer.get());
  }
  SDL_RenderPresent(m_renderer.get());
}

void Browser::style(Item *node, Parser::StyleSheet &rules) {
  //--NOTE: This inherite rules applies to all elements by default, and
  //         is overidden by ANY other rules applied to the elements themseleves
  for (auto &property : INHERITED_PROPERTIES) {
    if (node->m_parent) {
      node->m_style[property.first] = node->m_parent->m_style[property.first];
    } else {
      node->m_style[property.first] = property.second;
    }
  }
  //--NOTE: This one's from the style sheet!
  for (auto &rule : rules) {
    if (!rule.selector->matches(node))
      continue;
    for (auto &property : rule.property) {
      node->m_style[property.first] = property.second;
    }
  }
  if (node->getType() == ItemType::TAG &&
      static_cast<Tag *>(node)->m_attributes.contains("style")) {

    //--INFO: This one's inline styling
    Tag *tag = static_cast<Tag *>(node);
    auto pairs = Parser::CSSParser(tag->m_attributes["style"]).body();
    for (auto pair : pairs) {
      tag->m_style[pair.first] = pair.second;
    }
    //--NOTE: Resolving font size to absolute pixels instead of %
  }
  if (node->m_style.contains("font-size") &&
      node->m_style["font-size"].ends_with('%')) {
    std::string parent_font_size{};
    if (node->m_parent && node->m_parent->m_style.contains("font-size")) {
      parent_font_size = node->m_parent->m_style["font-size"];
    } else {
      parent_font_size = INHERITED_PROPERTIES["font-size"];
    }
    auto &str_fontS = node->m_style["font-size"];
    float font_frac =
        std::stof(str_fontS.substr(0, str_fontS.size() - 1)) / 100;
    float parent_px =
        std::stof(parent_font_size.substr(0, parent_font_size.size() - 2));
    node->m_style["font-size"] = std::to_string(font_frac * parent_px) + "px";
  }
  for (auto &child : node->m_children) {
    style(child.get(), rules);
  }
}

std::vector<std::string_view>
Browser::get_links(const std::vector<Item *> &list) {
  std::vector<std::string_view> links{};
  for (auto *node : list) {
    if ((node->getType() != ItemType::TAG) || node->m_text != "link")
      continue;
    auto *tag = static_cast<Tag *>(node);
    auto &attributes = tag->m_attributes;
    if (attributes.contains("rel") && attributes["rel"] == "stylesheet" &&
        attributes.contains("href"))
      links.push_back(tag->m_attributes["href"]);
  }
  return links;
}

void Browser::click(SDL_MouseButtonEvent &event) {
  float x = event.x;
  float y = event.y + m_scroll_y;
  std::vector<Layout::Layout *> objects{};
  auto list = hlp::tree_to_list(m_document.get());
  for (auto *obj : list) {
    if ((obj->m_start_x <= x && (x < (obj->m_start_x + obj->m_width))) &&
        (obj->m_start_y <= y && (y < (obj->m_start_y + obj->m_height)))) {
      objects.push_back(obj);
    }
  }
  if (objects.empty())
    return;
  auto *elt = objects.back()->m_node;
  while (elt) {
    if (elt->getType() == ItemType::TAG) {
      auto *tag = static_cast<Tag *>(elt);
      if (tag->m_text == "a" && tag->m_attributes.contains("href")) {
        URL url = m_url.resolve(tag->m_attributes["href"]);
        load(std::move(url));
        return;
      }
    }
    elt = elt->m_parent;
  }
}

#include "Browser.hpp"
#include "Parser.hpp"
#include "helpers.hpp"
#include "layout.hpp"
#include "url.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>

using namespace browser;

Tab::Tab(TabContext tctx, float l_tab_height) {
  tab_height = l_tab_height;
  ctx = std::move(tctx.lctx);
  m_engine = tctx.textEngine;
  m_fontCache = tctx.fontCache;
  m_height = tctx.size.height;
  m_width = tctx.size.width;
}

void Tab::paint_tree(Layout::Layout *layoutNode) {
  if (layoutNode->should_paint()) {
    auto displayVec = layoutNode->paint();
    for (auto &item : displayVec) {
      m_displayItems.push_back(std::move(item));
    }
  }
  for (auto &child : layoutNode->m_children) {
    paint_tree(child.get());
  }
}

void Tab::load(URL url) {
  m_scroll_y = 0; // resetting scroll to top of page everytime new page's loaded
  m_history.push_back(std::move(url));
  URL &l_url = m_history.back();
  std::string response = l_url.request();
  Parser::HTMLParser parser{response};
  m_rootNode.reset(parser.parse()); // layout has to own the root node...

  auto default_css = hlp::get_default_CSS();
  m_rules.clear();
  m_rules = Parser::CSSParser(default_css).parse();
  std::vector<std::string_view> css_links =
      get_links(hlp::tree_to_list(m_rootNode.get()));
  for (auto link : css_links) {
    auto style_url = l_url.resolve(link);
    std::string body{};
    try {
      body = style_url.request();
    } catch (std::exception &exc) {
      continue;
    }
    auto source = Parser::CSSParser(body).parse();
    m_rules.insert(m_rules.end(), std::make_move_iterator(source.begin()),
                   std::make_move_iterator(source.end()));
  }
  render();
  return;
}

// Responsible for styling the DOM, laying it out and collecting displayItems
void Tab::render() {
  auto cascade_priority = [](Parser::StyleRule &rule1,
                             Parser::StyleRule &rule2) {
    return rule1.selector->priority < rule2.selector->priority;
  };

  std::ranges::sort(m_rules, cascade_priority);
  style(m_rootNode.get(), m_rules);
  m_document = std::make_unique<Layout::DocumentLayout>(m_rootNode.get());
  m_document->layout(ctx);
  m_displayItems.clear();
  paint_tree(m_document.get());
}

// NOTE: offset accounts for all the chrome items on the top and forces all
// DrawItems to be drawm below it
void Tab::draw(SDL_Renderer *renderer, float offset) {
  for (size_t i{}; i < m_displayItems.size(); ++i) {
    auto &cmd = m_displayItems[i];
    if (cmd->original.top > m_scroll_y + tab_height)
      break; // if u below the screen just stop
    if (cmd->original.bottom < m_scroll_y)
      continue;
    cmd->execute(m_scroll_y - offset, renderer);
  }
}

void Tab::style(Item *node, Parser::StyleSheet &rules) {
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

std::vector<std::string_view> Tab::get_links(const std::vector<Item *> &list) {
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

void Tab::click(float x, float y, SDL_Window *window) {
  blur();
  URL &l_url = m_history.back();
  y += m_scroll_y;
  std::vector<Layout::Layout *> objects{};
  auto list = hlp::tree_to_list(m_document.get());
  for (auto *obj : list) {
    if ((obj->m_start_x <= x && (x < (obj->m_start_x + obj->m_width))) &&
        (obj->m_start_y <= y && (y < (obj->m_start_y + obj->m_height)))) {
      objects.push_back(obj);
    }
  }
  Item *elt;
  if (objects.empty()) {
    elt = nullptr;
  } else {
    elt = objects.back()->m_node;
  }
  while (elt) {
    if (elt->getType() == ItemType::TAG) {
      auto *tag = static_cast<Tag *>(elt);
      if (tag->m_text == "a" && tag->m_attributes.contains("href")) {
        URL url = l_url.resolve(tag->m_attributes["href"]);
        load(std::move(url));
        return;
      } else if (tag->m_text == "input") {
        m_focus = elt;
        m_focus->focus = true;
        SDL_StartTextInput(window);
        tag->m_attributes["value"] = "";
        render();
      }
    }
    elt = elt->m_parent;
  }
}

// NOTE: Book has it named scrolldown but since we support both up and down I
// just made it scroll
void Tab::scroll(float turn) {
  m_max_y = std::max(m_document->m_height + 2.0f * static_cast<float>(VSTEP) -
                         tab_height,
                     0.0f);
  m_scroll_y -= turn * 40.0f;
  if (m_scroll_y < 0.0f)
    m_scroll_y = 0.0f;
  if (m_scroll_y > m_max_y) {
    m_scroll_y = m_max_y;
  }
}

void Tab::go_back() {
  if (m_history.size() > 1) {
    // pop back current page's URL
    m_history.pop_back();
    // Get the prev page's string URL
    //--WARNING: We can't use the stale URL object cuz the connection's already
    // closed when it did .request back then
    URL url = URL(m_history.back().m_url);
    // remove the empty index
    m_history.pop_back();
    load(std::move(url));
  }
}

void Tab::handle_special_keys(SDL_Keycode key) {
  if (m_focus == nullptr)
    return;
  auto *tag = static_cast<Tag *>(m_focus);
  switch (key) {
  case SDLK_BACKSPACE: {
    if (!tag->m_attributes["value"].empty()) {
      tag->m_attributes["value"].pop_back();
    }
    break;
  }
  }
  render();
}

void Tab::handle_input(std::string str) {
  if (m_focus == nullptr)
    return;
  auto *tag = static_cast<Tag *>(m_focus);
  tag->m_attributes["value"] += str;
  render();
  return;
}

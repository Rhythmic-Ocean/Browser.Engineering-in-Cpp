#include "helpers.hpp"
#include "layout.hpp"
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

using namespace Layout;

/*--NOTE: DrawItem's member function definitions
          Each item is rendered individually here
*/
void DrawText::execute(float scroll_y, SDL_Renderer *renderer) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  float cur_scroll_y{original.top - scroll_y};
  if (!TTF_DrawRendererText(m_text, m_left, cur_scroll_y)) {
    SDL_Log("Failed to draw text at item %s: %s\n", m_text->text,
            SDL_GetError());
  }
}

void DrawRect::execute(float scroll_y, SDL_Renderer *renderer) {
  SDL_FRect rect = {original.left, original.top - scroll_y,
                    original.right - original.left,
                    original.bottom - original.top};

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

  if (!SDL_RenderFillRect(renderer, &rect)) {
    SDL_Log("Failed to render layout rectangle: %s\n", SDL_GetError());
    throw WindowException("Can't make rectangles!!" +
                          std::string(SDL_GetError()));
  }
}

void DrawOutline::execute(float scroll_y, SDL_Renderer *renderer) {
  for (auto &border : borders) {
    border.y = original.top - scroll_y;
  }
  // for bottom border
  borders[1].y = original.bottom - borders[1].h - scroll_y;
  inner.y = original.top - scroll_y;
  SDL_SetRenderDrawColor(renderer, innerColor.r, innerColor.g, innerColor.b,
                         innerColor.a);
  if (!SDL_RenderFillRect(renderer, &inner)) {
    SDL_Log("Failed to render layout outline inner: %s\n", SDL_GetError());
    throw WindowException("Can't make inners for outline!!");
  }
  SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b,
                         borderColor.a);
  if (!SDL_RenderFillRects(renderer, borders.data(), 4)) {
    SDL_Log("Failed to render layout outline borders: %s\n", SDL_GetError());
    throw WindowException("Can't make borders for outline!!");
  }
}

void DrawLine::execute(float scroll_y, SDL_Renderer *renderer) {
  y1 = original.top - scroll_y;
  y2 = original.bottom - scroll_y;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  if (!SDL_RenderLine(renderer, x1, y1, x2, y2)) {
    SDL_Log("Failed to render  line: %s\n", SDL_GetError());
    throw WindowException("Can't make a line!!");
  }
}

//--NOTE: Layout's member function definitions

void DocumentLayout::layout(LayoutContext &ctx) {
  BlockLayout *child = new BlockLayout(m_node, this, nullptr);
  m_children.emplace_back(child);
  m_width = ctx.windowWidth - 2 * HSTEP;
  m_start_x = HSTEP;
  m_start_y = VSTEP;
  child->layout(ctx);
  m_height = child->m_height;
}

std::vector<std::unique_ptr<DrawItem>> DocumentLayout::paint() { return {}; }

LayoutType BlockLayout::layout_mode() {
  auto checkBlockTag = [](std::vector<std::unique_ptr<Item>> &children) {
    for (auto &child : children) {
      if (std::ranges::find(BLOCK_ELEMENTS, child->m_text) !=
          BLOCK_ELEMENTS.end())
        return true;
    }
    return false;
  };
  // if plain text, it's inline by default
  if (m_node->getType() == ItemType::TEXT)
    return LayoutType::INLINE;
  // if it's a tag and it's children are of block type it gets block
  else if (m_node->getType() == ItemType::TAG &&
           checkBlockTag(m_node->m_children)) {
    return LayoutType::BLOCK;
    // if the node has children of if it's and input tag
  } else if (!m_node->m_children.empty() || m_node->m_text == "input")
    return LayoutType::INLINE;
  // web developer's mistake for having an empty block type tag!!
  else
    return LayoutType::BLOCK;
}

void BlockLayout::layout(LayoutContext &ctx) {
  m_start_x = m_parent->m_start_x;
  m_width = m_parent->m_width;
  if (m_previous) {
    m_start_y = m_previous->m_start_y + m_previous->m_height;
  } else {
    m_start_y = m_parent->m_start_y;
  }
  LayoutType mode = layout_mode();
  if (mode == LayoutType::BLOCK) {
    Layout *previous = nullptr;
    for (auto &m_child : m_node->m_children) {
      m_children.emplace_back(
          std::make_unique<BlockLayout>(m_child.get(), this, previous));
      previous = m_children.back().get();
    }
  } else {
    new_line();
    recurse(m_node, ctx);
  }

  for (auto &child : m_children) {
    child->layout(ctx);
  }

  for (auto &child : m_children) {
    m_height += child->m_height;
  }
}

/*
--WARNING: The multi-byte parsing algorithm in the following functioin was
           generated with heavy AI assistance. Extensive line-by-line notes for
           explanative purpose.
 */
//--INFO: relative x postion of the text gets set in word/make_word.
//        absolute x and y position gets set at flush()
void BlockLayout::word(Item *node, LayoutContext &ctx) {
  std::string &str = node->m_text;
  std::string word{};
  for (size_t c = 0; c < str.size(); ++c) {
    unsigned char byte = static_cast<unsigned char>(str[c]);
    /*--NOTE: Collapsing consecutive whitespaces into a single space.
              Important to note that the whitespaces we see in webpages are
              the result of tags, not excess ones from plain text in a .html
              file!!*/
    if (std::isspace(byte)) {
      if (!word.empty()) {
        make_word(node, std::move(word), ctx);
        word.clear();
      }
      while (c < str.size() &&
             std::isspace(static_cast<unsigned char>(str[c]))) {
        ++c;
      }
      --c;
      continue;
    }
    /*--NOTE: To support multi-byte unicode characters. Multi-byte characters
              stores how many bytes it's made up of in it's first byte as
              follows:
              - 0xxxxxxx -> 1 byte chars has first bit set 0, rest of 7 bits
                store info.
                That's why we & the byte with 0x10000000, and if it gives 0x00
                it's a one byte char
              - 11xxxxxx -> this first byte pattern is for 2 byte chars
              - 111xxxxx -> this first byte pattern is for 3 byte chars
              - 1111xxxx -> this first byte pattern is for 4 byte chars
     */
    if ((byte & 0x80) == 0x00) { // & with 0x10000000
      word += str[c];
    } else {
      if (!word.empty()) { // if the char is no 1 byte then we immediatly
                           // clear the buffer and start a new item
        make_word(node, std::move(word), ctx);
        word.clear();
      }
      size_t char_length = 2;
      if ((byte & 0xF0) == 0xE0) // & w/ 0x11110000
        char_length = 3;
      else if ((byte & 0xF8) == 0xF0) //& w/ 0x11111000
        char_length = 4;
      std::string utf8_char = str.substr(c, char_length);
      // each of the multi byte char are treated as seperate display item
      make_word(node, std::move(utf8_char), ctx);
      utf8_char.clear();
      c += (char_length - 1); // as the enclosing for loop performs ++c next
    }
  }
  // Flush the remaining chars from buffer
  if (!word.empty())
    make_word(node, std::move(word), ctx);
  word.clear();
}

void BlockLayout::input(Item *node, LayoutContext &ctx) {
  float w = INPUT_WIDTH_PX;
  if (m_cursor_x + w > m_width)
    new_line();
  auto *layout = m_children.back().get();
  if (layout->getType() != LayoutType::LINE)
    throw LayoutException(
        "Error at BlockLayout::make_word(). Type is not LineLayout.");
  auto *line = static_cast<LineLayout *>(layout);
  TextLayout *previous_word{nullptr};
  if (!line->m_children.empty()) {
    //--NOTE: Only TextLayout are gonna be LineLayout's children so no need to
    //        check
    previous_word = static_cast<TextLayout *>(line->m_children.back().get());
  }
  auto new_input = std::make_unique<InputLayout>(node, line, previous_word);
  line->m_children.push_back(std::move(new_input));

  auto &weight = m_node->m_style["font-weight"];
  auto &size = m_node->m_style["font-size"];
  auto *font = ctx.fontCache->get_font(FontCache::get_weight(weight),
                                       FontCache::get_size(size));
  int iWidth;
  TTF_GetStringSize(font, " ", 1, &iWidth, nullptr);
  m_cursor_x += w + static_cast<float>(iWidth);
  return;
}

void BlockLayout::recurse(Item *root, LayoutContext &ctx) {
  if (root->getType() == ItemType::TEXT) {
    word(root, ctx);
  } else {
    if (root->m_text == "br")
      new_line();
    else if (root->m_text == "input" || root->m_text == "button")
      input(root, ctx);
    else {
      for (auto &child : root->m_children) {
        recurse(child.get(), ctx);
      }
    }
  }
}

//--INFO: relative x postion of the text gets set in word/make_word.
//        absolute x and y position gets set at flush()
void BlockLayout::make_word(Item *node, std::string word, LayoutContext &ctx) {

  // getting formatting info from current Block Node
  auto &weight = node->m_style["font-weight"];
  //--WARNING: Only have one style for nw, stick w/ it. So we ignore this
  [[maybe_unused]] auto &style = node->m_style["font-style"];
  auto &size = node->m_style["font-size"];

  // creating the TTF_Text and coloring it
  auto *font = ctx.fontCache->get_font(FontCache::get_weight(weight),
                                       FontCache::get_size(size));
  auto *txt{TTF_CreateText(ctx.textEngine, font, word.c_str(), word.size())};
  auto *space{TTF_CreateText(ctx.textEngine, font, " ", 1)};
  if (!txt || !space) {
    SDL_Log("Couldn't create text: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't create text: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }

  // Getting the word's dimensions
  int iHeight{};
  int iWidth{};

  if (!TTF_GetTextSize(txt, &iWidth, &iHeight)) {
    SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't calculate string size of: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  float width = static_cast<float>(iWidth);
  [[maybe_unused]] float height = static_cast<float>(iHeight);

  // Getting the space's dimensions
  if (!TTF_GetTextSize(space, &iWidth, &iHeight)) {
    SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't calculate string size of: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  float Swidth = static_cast<float>(iWidth);
  [[maybe_unused]] float Sheight = static_cast<float>(iHeight);
  // If cursor moves past this block's width change line
  if (m_cursor_x + width > m_width) {
    new_line();
  }
  m_cursor_x += width + Swidth;

  /*Grabbing the ongoing line layout, creating a new txt layout and adding it to
   * the line*/
  auto *layout = m_children.back().get();
  if (layout->getType() != LayoutType::LINE)
    throw LayoutException(
        "Error at BlockLayout::make_word(). Type is not LineLayout.");
  auto *line = static_cast<LineLayout *>(layout);
  TextLayout *previous_word{nullptr};
  if (!line->m_children.empty()) {
    //--NOTE: Only TextLayout are gonna be LineLayout's children so no need to
    //        check
    previous_word = static_cast<TextLayout *>(line->m_children.back().get());
  }
  auto new_word =
      std::make_unique<TextLayout>(node, std::move(word), line, previous_word);
  line->m_children.push_back(std::move(new_word));
  return;
}

std::vector<std::unique_ptr<DrawItem>> BlockLayout::paint() {
  std::vector<std::unique_ptr<DrawItem>> cmds{};
  std::string bg_color = "transparent";
  if (m_node->m_style.contains("background-color")) {
    bg_color = m_node->m_style["background-color"];
    cmds.emplace_back(
        std::make_unique<DrawRect>(self_rect(), parse_color(bg_color)));
  }
  return cmds;
}

void DocumentLayout::pprint() {
  std::cout << "NOTHING" << std::endl;
  m_children[0]->pprint();
}

void BlockLayout::pprint() {
  if (layout_mode() == LayoutType::BLOCK) {
    std::cout << "BLOCK " << m_node->m_text << std::endl;
    for (auto &node : m_children) {
      static_cast<BlockLayout *>(node.get())->pprint();
    }
  } else {
    for (auto &child : m_children) {
      child.get()->pprint();
    }
  }
}

void BlockLayout::new_line() {
  m_cursor_x = 0;
  LineLayout *lastLine{nullptr};
  if (!m_children.empty()) {
    lastLine = static_cast<LineLayout *>(m_children.back().get());
  }
  auto new_line = std::make_unique<LineLayout>(m_node, this, lastLine);
  m_children.push_back(std::move(new_line));
}

Rect BlockLayout::self_rect() {
  return Rect{m_start_x, m_start_y, m_start_x + m_width, m_start_y + m_height};
}

bool Rect::contains_point(float x, float y) {
  return x >= left && x < right && y >= top && y < bottom;
};

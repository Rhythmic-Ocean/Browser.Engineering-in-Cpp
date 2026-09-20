#include "layout.hpp"
#include "helpers.hpp"
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
  float cur_scroll_y{m_top - scroll_y};
  if (!TTF_DrawRendererText(m_text, m_left, cur_scroll_y)) {
    SDL_Log("Failed to draw text at item %s: %s\n", m_text->text,
            SDL_GetError());
  }
}

void DrawRect::execute(float scroll_y, SDL_Renderer *renderer) {
  rect.y = m_top - scroll_y;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  if (!SDL_RenderFillRect(renderer, &rect)) {
    SDL_Log("Failed to render layout rectangle: %s\n", SDL_GetError());
    throw WindowException("Can't make rectangles!!" +
                          std::string(SDL_GetError()));
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

std::vector<DrawItem *> DocumentLayout::paint() { return {}; }

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
    // all of this node's children don't form block layout!
  } else if (!m_node->m_children.empty())
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
    m_cursor_x = 0;
    m_cursor_y = 0;
    m_fontWeight = FontWeight::REGULAR;
    m_fontSize = 16;
    m_font = ctx.fontCache->get_font(m_fontWeight, m_fontSize);
    recurse(m_node, ctx);
    flush();
  }

  for (auto &child : m_children) {
    child->layout(ctx);
  }

  if (mode == LayoutType::BLOCK) {
    for (auto &child : m_children) {
      m_height += child->m_height;
    }
  }
  if (mode == LayoutType::INLINE) {
    m_height = m_cursor_y;
  }
}

void BlockLayout::open_tag(const std::string &tag) {
  if (tag == "i") {
    if (m_fontWeight == FontWeight::BOLD)
      m_fontWeight = FontWeight::BOLD_ITALICS;
    else
      m_fontWeight = FontWeight::ITALICS;
  } else if (tag == "b") {
    if (m_fontWeight == FontWeight::ITALICS)
      m_fontWeight = FontWeight::BOLD_ITALICS;
    else
      m_fontWeight = FontWeight::BOLD;
  } else if (tag == "small")
    m_fontSize = 10;
  else if (tag == "big")
    m_fontSize += 20;
  else if (tag == "br")
    flush();
  else if (tag == "pre") {
    std::cout << "here" << std::endl;
  }
}

void BlockLayout::close_tag(const std::string &tag) {
  if (tag == "i") {
    if (m_fontWeight == FontWeight::BOLD_ITALICS)
      m_fontWeight = FontWeight::BOLD;
    else
      m_fontWeight = FontWeight::REGULAR;
  } else if (tag == "b") {
    if (m_fontWeight == FontWeight::BOLD_ITALICS)
      m_fontWeight = FontWeight::ITALICS;
    else
      m_fontWeight = FontWeight::REGULAR;
  } else if (tag == "small")
    m_fontSize += 10;
  else if (tag == "big")
    m_fontSize = 20;
  else if (tag == "p") {
    flush();
    m_cursor_y += VSTEP;
  }
}

/*
--WARNING: The multi-byte parsing algorithm in the following functioin was
           generated with heavy AI assistance. Extensive line-by-line notes for
           explanative purpose.
 */
//--INFO: relative x postion of the text gets set in process_text/make_display.
//        absolute x and y position gets set at flush()
void BlockLayout::process_text(const std::string &str, LayoutContext &ctx) {
  std::string word{};
  for (size_t c = 0; c < str.size(); ++c) {
    unsigned char byte = static_cast<unsigned char>(str[c]);
    /*--NOTE: Collapsing consecutive whitespaces into a single space.
              Important to note that the whitespaces we see in webpages are
              the result of tags, not excess ones from plain text in a .html
              file!!*/
    if (std::isspace(byte)) {
      if (!word.empty()) {
        m_line.emplace_back(std::move(make_display(word, ctx)));
      }
      while (c < str.size() &&
             std::isspace(static_cast<unsigned char>(str[c]))) {
        ++c;
      }
      --c;
      std::string space_str = " ";
      m_line.emplace_back(std::move(make_display(space_str, ctx)));
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
        m_line.emplace_back(std::move(make_display(word, ctx)));
      }
      size_t char_length = 2;
      if ((byte & 0xF0) == 0xE0) // & w/ 0x11110000
        char_length = 3;
      else if ((byte & 0xF8) == 0xF0) //& w/ 0x11111000
        char_length = 4;
      std::string utf8_char = str.substr(c, char_length);
      // each of the multi byte char are treated as seperate display item
      m_line.emplace_back(std::move(make_display(utf8_char, ctx)));
      c += (char_length - 1); // as the enclosing for loop performs ++c next
    }
  }
  // Flush the remaining chars from buffer
  if (!word.empty())
    m_line.emplace_back(std::move(make_display(word, ctx)));
}

void BlockLayout::recurse(Item *root, LayoutContext &ctx) {
  if (root->getType() == ItemType::TEXT) {
    process_text(root->m_text, ctx);
  } else {
    open_tag(root->m_text);
    for (auto &child : root->m_children) {
      recurse(child.get(), ctx);
    }
    close_tag(root->m_text);
  }
}

//--INFO: relative x postion of the text gets set in process_text/make_display.
//        absolute x and y position gets set at flush()
PositionedText BlockLayout::make_display(std::string &word,
                                         LayoutContext &ctx) {
  int h{};
  int w{};
  auto *font = ctx.fontCache->get_font(m_fontWeight, m_fontSize);
  auto *txt{TTF_CreateText(ctx.textEngine, font, word.c_str(), word.size())};

  if (!txt) {
    SDL_Log("Couldn't create text: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't create text: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  TTF_SetTextColor(txt, 255, 255, 255, 255);
  if (!TTF_GetTextSize(txt, &w, &h)) {
    SDL_Log("Couldn't calculate text size of: %s. Error: %s\n", word.c_str(),
            SDL_GetError());
    throw WindowException("Couldn't calculate string size of: " + word +
                          ". Error: " + std::string(SDL_GetError()));
  }
  if (m_cursor_x + w > m_width) {
    flush();
  }
  m_cursor_x += w;
  word.clear();
  return {txt, m_cursor_x - w,
          0.0f}; //--WARNING: The y coords 0.0f is just a placeholder, the
                 //               actual will be calculated at flush()
}

//--INFO: relative x postion of the text gets set in process_text/make_display.
//        absolute x and y position gets set at flush()

//--WARNING: flush() should still happen even if m_line is empty!! Important to
//            change lines for layout/ linebreak tags!
void BlockLayout::flush() {
  int max_ascent{}, max_descent{}, max_lineskip{};
  getExtremes(max_ascent, max_descent, max_lineskip, m_line);
  float baseline = m_cursor_y + 1.25 * max_ascent;
  for (auto &word : m_line) {
    TTF_Font *font = TTF_GetTextFont(word.text.get());
    float font_ascent = TTF_GetFontAscent(font);
    word.start_x += m_start_x; // absolute position of x
    word.start_y +=
        (m_start_y + baseline - font_ascent); // relative pos for y??
    m_displayList.push_back(std::move(word));
  }
  m_cursor_x = 0;
  m_line.clear();
  m_cursor_y = baseline + 1.25 * max_descent;
}

std::vector<DrawItem *> BlockLayout::paint() {
  std::vector<DrawItem *> cmds{};
  if (m_node->getType() == ItemType::TAG && m_node->m_text == "pre") {
    std::string color = "grey";
    cmds.emplace_back(
        new DrawRect{m_start_x, m_start_y, m_width, m_height, color});
  }

  if (layout_mode() == LayoutType::INLINE) {
    for (auto &item : m_displayList) {
      cmds.emplace_back(
          new DrawText{item.text.get(), item.start_x, item.start_y});
    }
  }
  return cmds;
}

void BlockLayout::getExtremes(int &max_ascent, int &max_descent,
                              int &max_lineskip,
                              std::vector<PositionedText> &line) {
  for (auto &word : line) {
    TTF_Font *font = TTF_GetTextFont(word.text.get());
    max_ascent = std::max(max_ascent, TTF_GetFontAscent(font));
    max_descent = std::max(max_descent, std::abs(TTF_GetFontDescent(font)));
    max_lineskip = std::max(max_lineskip, TTF_GetFontLineSkip(font));
  }
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
    std::cout << "INLINE" << m_node->m_text << std::endl;
    for (auto &child : m_node->m_children) {
      hlp::print_tree(child.get());
    }
  }
}

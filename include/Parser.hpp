#pragma once

#include "helpers.hpp"
#include <array>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace Parser {
enum class SelectorType { TAG, DESCENDANT };

struct Selector {
  virtual SelectorType getType() = 0;
  virtual bool matches(Item *node) = 0;
};

using Property = std::unordered_map<std::string, std::string>;
struct StyleRule {
  std::unique_ptr<Selector> selector;
  Property property;
  StyleRule(std::unique_ptr<Selector> &selector, Property &property)
      : selector{selector.release()}, property{property} {}
};

using StyleSheet = std::vector<StyleRule>;

struct TagSelector : public Selector {
  std::string tag;

  bool matches(Item *node) {
    return node->getType() == ItemType::TAG && tag == node->m_text;
  }

  SelectorType getType() { return SelectorType::TAG; }
  TagSelector(const std::string &tag) : tag{tag} {};
};

//--NOTE: Each selector owns it's descendent under unique ptr, but not the
// ancenstor

struct DescendantSelector : public Selector {
  std::unique_ptr<Selector> ancestor{};
  std::unique_ptr<Selector> descendant{};

  SelectorType getType() { return SelectorType::DESCENDANT; }
  bool matches(Item *node) {
    if (!descendant->matches(node))
      return false;
    while (node->m_parent) {
      if (ancestor->matches(node->m_parent))
        return true;
      node = node->m_parent;
    }
    return false;
  }

  DescendantSelector(std::unique_ptr<Selector> &ancestor,
                     std::unique_ptr<Selector> &descendant)
      : ancestor{ancestor.release()}, descendant{descendant.release()} {}
};

class CSSParser {
  constexpr static std::string VALID_SYMB = "#-.%";

  std::string_view m_body{};
  size_t index{};

  void whiteSpace();
  std::string word();
  void literal(char literal);
  std::pair<std::string, std::string> pair();
  std::optional<char> ignore_until(const std::string &literals);
  Selector *selector();

public:
  CSSParser(std::string_view body) : m_body{body} {}
  std::unordered_map<std::string, std::string> body();
  StyleSheet parse();
};

class HTMLParser {
  /*
   *--NOTE: std::string_view does not own these strings, but it's usually only
   * dangerous when they live in the stack, here it's in the compiled binary
   * so doesn't matter much since it's not a temporary
   * */
  constexpr static std::array<std::string_view, 14> SELF_CLOSING = {
      "area",  "base", "br",   "col",   "embed",  "hr",    "img",
      "input", "link", "meta", "param", "source", "track", "wbr"};
  constexpr static std::array<std::string_view, 3> AFTER_HTML_TAGS = {
      "head", "body", "/html"};
  constexpr static std::array<std::string_view, 9> HEAD_TAGS{
      "base", "basefont", "bgsound", "noscript", "link",
      "meta", "title",    "style",   "script",
  };
  std::vector<Item *> m_unfinished{};
  std::string &m_body;
  void add_text(std::string &&text);
  void add_tag(std::string &&tag);
  void implcit_tag(const std::string &tag);
  std::pair<std::string, std::unordered_map<std::string, std::string>>
  get_attributes(std::string &text);
  Item *finish();
  std::vector<std::string> attrib_splitter(std::string &str);
  std::vector<std::string> parse_attrib(std::string &str);

public:
  HTMLParser(std::string &body);
  Item *parse();
};
} // namespace Parser

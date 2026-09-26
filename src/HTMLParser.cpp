#include "Parser.hpp"
#include "helpers.hpp"
#include <cctype>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>

using namespace Parser;

HTMLParser::HTMLParser(std::string &body) : m_body{body} {}
Item *HTMLParser::parse() {
  std::string word{};
  bool in_tag = false;
  for (size_t c = 0; c < m_body.size(); ++c) {
    if (m_body[c] == '<') {
      in_tag = true;
      add_text(std::move(word));
      word.clear();
    } else if (m_body[c] == '>') {
      in_tag = false;
      add_tag(std::move(word));
      word.clear();
    } else if (!in_tag && m_body[c] == '&' && m_body.size() - c >= 4 &&
               m_body.compare(c, 4, "&lt;") == 0) {
      word += "<";
      c += 3; // skip past "lt;" (loop's ++c handles the last +1)
    } else if (!in_tag && m_body[c] == '&' && m_body.size() - c >= 4 &&
               m_body.compare(c, 4, "&gt;") == 0) {
      word += '>';
      c += 3;
    } else
      word += m_body[c];
  }
  if (!word.empty())
    add_text(std::move(word));
  return finish();
}
void HTMLParser::add_text(std::string &&text) {
  if (hlp::strip(text).size() == 0)
    return;
  implcit_tag("");
  auto *parent = m_unfinished.back();
  Item *node = new Text(std::move(text), parent);
  parent->m_children.emplace_back(node);
}

/*
--NOTE: The open tag is first put into the unfinished bucket, it's only put
  inside the parent's m_children node after it's closed.
 * */
void HTMLParser::add_tag(std::string &&text) {
  auto [tag, attributes] = get_attributes(text);
  if (tag.starts_with('!')) // ignoring !doctype stuff and comments too
    return;
  implcit_tag(tag);
  if (tag.starts_with('/')) {
    if (m_unfinished.size() ==
        1) // the very last last tag's an edge case cuz it got no parent so we
           // don't add it anywhere
      return;
    auto *node = m_unfinished.back();
    m_unfinished.pop_back();
    auto *parent = m_unfinished.back();
    parent->m_children.emplace_back(node);
  } else if (std::ranges::find(SELF_CLOSING, tag) != SELF_CLOSING.end()) {
    auto *parent = m_unfinished.back();
    Item *node = new Tag(std::move(tag), parent, std::move(attributes));
    parent->m_children.emplace_back(node);
  } else {
    Item *parent;
    if (m_unfinished
            .empty()) { // the very first open tag is an edge-case w/o a parent
      parent = nullptr;
    } else
      parent = m_unfinished.back();
    Item *node = new Tag(std::move(tag), parent, std::move(attributes));
    m_unfinished.push_back(node);
  }
}

std::pair<std::string, std::unordered_map<std::string, std::string>>
HTMLParser::get_attributes(std::string &text) {
  auto parts = parse_attrib(text);
  std::unordered_map<std::string, std::string> attributes{};
  auto tag = std::string(parts[0]);
  hlp::casefold(tag);
  for (auto &attrpair : parts | std::views::drop(1)) {
    if (attrpair.find('=') != std::string_view::npos) {
      auto valPairs = hlp::split(attrpair, "=", 1);
      auto key = std::string(valPairs[0]);
      auto value = (valPairs[1]);
      if (value.size() > 2 && (value.front() == '\'' || value.front() == '"')) {
        value = value.substr(1, value.size() - 2);
      }
      std::string val = std::string(value);
      hlp::casefold(key);
      attributes[key] = std::move(val);
    } else {
      auto key = std::string(attrpair);
      hlp::casefold(key);
      key = hlp::strip(key);
      attributes[key] = "";
    }
  }
  return {std::move(tag), std::move(attributes)};
}

void HTMLParser::implcit_tag(const std::string &tag) {
  std::vector<std::reference_wrapper<std::string>> open_tags{};
  while (true) {
    for (auto &node : m_unfinished) {
      open_tags.push_back(node->m_text);
    }
    if (open_tags.empty() && tag != "html") {
      add_tag("html");
    } else if (open_tags.size() == 1 && open_tags.back().get() == "html" &&
               std::ranges::find(AFTER_HTML_TAGS, tag) ==
                   AFTER_HTML_TAGS.end()) {
      if (std::ranges::find(HEAD_TAGS, tag) != HEAD_TAGS.end()) {
        add_tag("head");
      } else {
        add_tag("body");
      }
    } else if ((open_tags.size() == 2 && open_tags.back().get() == "head") &&
               (tag != "/head" &&
                std::ranges::find(HEAD_TAGS, tag) == HEAD_TAGS.end())) {
      add_tag("/head");
    } else
      break;
  }
}

Item *HTMLParser::finish() {
  if (m_unfinished.empty())
    implcit_tag("");
  while (m_unfinished.size() > 1) {
    auto *node = m_unfinished.back();
    m_unfinished.pop_back();
    auto *parent = m_unfinished.back();
    parent->m_children.emplace_back(node);
  }
  Item *ancestor = m_unfinished.back();
  m_unfinished.pop_back();
  return ancestor;
}

void whiteSpace(std::string &body, int &indx) {
  while (indx < body.size() &&
         std::isspace(static_cast<unsigned char>(body[indx])))
    ++indx;
  return;
}

std::string word(std::string &body, int &indx, bool inQuotes) {
  size_t start = indx;
  while (indx < body.size()) {
    if (inQuotes && std::isspace(static_cast<unsigned char>(body[indx]))) {
      ++indx;
    } else if (!std::isspace(static_cast<unsigned char>(body[indx])) &&
               body[indx] != '=' && body[indx] != '"') {
      ++indx;
    } else
      break;
  }
  if (indx <= start)
    ++indx;
  return std::string(body.substr(start, indx - start));
}

bool is_literal(std::string &body, int &indx, char l_literal) {
  if (indx < body.size() && body[indx] == l_literal) {
    ++indx;
    return true;
  }
  return false;
}

void literal(std::string &body, int &indx, char l_literal) {
  if (indx < body.size() && body[indx] != l_literal)
    throw WindowException("Failed catching literal: " + std::string{l_literal});
  ++indx;
}

std::optional<char> ignore_until(std::string &body, int &indx,
                                 const std::string &literals) {
  while (indx < body.size()) {
    if (std::ranges::find(literals, body[indx]) != literals.end())
      return body[indx];
    else
      ++indx;
  }
  return std::nullopt;
}

std::vector<std::string> HTMLParser::parse_attrib(std::string &body) {
  int indx = 0;
  int b_size = body.size();
  std::vector<std::string> attributes{};
  std::string l_word{};
  while (indx < b_size) {
    try {
      whiteSpace(body, indx);
      l_word += word(body, indx, false);
      whiteSpace(body, indx);
      if (!is_literal(body, indx, '=')) {
        attributes.push_back(l_word);
        l_word.clear();
        continue;
      }
      l_word += '=';
      whiteSpace(body, indx);
      literal(body, indx, '"');
      l_word += '"';
      l_word += word(body, indx, true);
      literal(body, indx, '"');
      l_word += '"';
      attributes.push_back(l_word);
      l_word.clear();
    } catch (WindowException &exception) {
      auto why = ignore_until(body, indx, "\"");
      if (why == '"') {
        literal(body, indx, '"');
        whiteSpace(body, indx);
        l_word.clear();
        continue;
      }
      if (!l_word.empty())
        attributes.push_back(l_word);
      else
        attributes.push_back("dummy");
      break;
    }
  }
  return attributes;
}

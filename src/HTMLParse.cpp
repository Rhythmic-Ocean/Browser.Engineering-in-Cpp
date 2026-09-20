#include "HTMLParse.hpp"
#include "helpers.hpp"
#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>

HTMLParse::HTMLParse(std::string &body) : m_body{body} {}
Item *HTMLParse::parse() {
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
void HTMLParse::add_text(std::string &&text) {
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
void HTMLParse::add_tag(std::string &&text) {
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
HTMLParse::get_attributes(std::string &text) {
  auto parts = attrib_splitter(text);
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

void HTMLParse::implcit_tag(const std::string &tag) {
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

Item *HTMLParse::finish() {
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

std::vector<std::string> HTMLParse::attrib_splitter(std::string &str) {
  int ptr{};
  bool inQuotes = false;
  std::string cur_str{};
  std::vector<std::string> finalAns{};
  while (ptr < str.size()) {
    if (std::isspace(str[ptr]) && !inQuotes) {
      if (!cur_str.empty())
        finalAns.push_back(std::move(cur_str));
      cur_str.clear();
      while (ptr < str.size() && std::isspace(str[ptr]))
        ++ptr;
    } else {
      if (str[ptr] == '\'' || str[ptr] == '"') {
        inQuotes = !inQuotes;
      }
      cur_str.push_back(str[ptr]);
    }
    ++ptr;
  }
  if (!cur_str.empty())
    finalAns.push_back(std::move(cur_str));
  return finalAns;
}

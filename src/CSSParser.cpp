#include "Parser.hpp"
#include "helpers.hpp"
#include <optional>
#include <unordered_map>

using namespace Parser;

void CSSParser::whiteSpace() {
  while (index < m_body.size() && std::isspace(m_body[index]))
    ++index;
}

std::string CSSParser::word() {
  size_t start{index};
  while (index < m_body.size()) {
    if (std::isalnum(m_body[index]) ||
        (std::ranges::find(VALID_SYMB, m_body[index]) != VALID_SYMB.end())) {
      ++index;
    } else
      break;
  }
  if (index <= start) {
    throw WindowException("Failed at CSSParser.word(). No word parsed.");
  }
  return std::string(m_body.substr(start, index - start));
}

void CSSParser::literal(char literal) {
  if (!(index < m_body.size()) || m_body[index] != literal) {
    throw WindowException(
        "Failed at CSSParser.literal(). Literal comparator failed.");
  }
  ++index;
}

std::pair<std::string, std::string> CSSParser::pair() {
  std::string property = std::string(word());
  whiteSpace();
  literal(':');
  whiteSpace();
  std::string value = word();
  hlp::casefold(property);
  return {property, value};
}

//--NOTE: the try-excpet here is REQUIRED FOR inline css parsion (cuz they don't
//        need to have ;)
Property CSSParser::body() {
  Property pairs{};
  while (index < m_body.size() && m_body[index]) {
    try {
      auto [property, value] = pair();
      pairs[property] = value;
      whiteSpace();
      literal(';');
      whiteSpace();
    } catch (WindowException exception) {
      auto why = ignore_until(";}");
      if (why == ';') {
        literal(';');
        whiteSpace();
      } else
        break;
    }
  }
  return pairs;
}

Selector *CSSParser::selector() {
  auto select = std::string(word());
  hlp::casefold(select);
  std::unique_ptr<Selector> out =
      std::make_unique<TagSelector>(std::move(select));
  whiteSpace();
  while (index < m_body.size() && m_body[index] != '{') {
    std::string tag = word();
    std::unique_ptr<Selector> descendant =
        std::make_unique<TagSelector>(std::move(tag));
    out = std::make_unique<DescendantSelector>(out, descendant);
    whiteSpace();
  }
  return out.release();
}

//--WARNING: Get rid of try-catch during testings
StyleSheet CSSParser::parse() {
  StyleSheet rules{};
  while (index < m_body.size()) {
    // try {
    whiteSpace();
    std::unique_ptr<Selector> l_selector;
    l_selector.reset(selector());
    literal('{');
    whiteSpace();
    Property l_body = body();
    literal('}');
    rules.emplace_back(StyleRule{l_selector, l_body});
    // } catch (WindowException exception) {
    //   auto why = ignore_until("}");
    //   if (why == '}') {
    //
    //     literal('}');
    //     whiteSpace();
    //   }
    // }
  }
  return rules;
}

std::optional<char> CSSParser::ignore_until(const std::string &literals) {
  while (index < m_body.size()) {
    if (std::ranges::find(literals, m_body[index]) != literals.end())
      return m_body[index];
    else
      ++index;
  }
  return std::nullopt;
}

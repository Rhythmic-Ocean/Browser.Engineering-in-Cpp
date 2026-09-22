#include "Parser.hpp"
#include "helpers.hpp"
#include <optional>
#include <unordered_map>

using namespace Parser;

void CSSParser::whiteSpace() {
  while (index < m_body.size() && std::isspace(m_body[index]))
    ++index;
}

std::string_view CSSParser::word() {
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
  return m_body.substr(start, index - start);
}

void CSSParser::literal(char literal) {
  if (!(index < m_body.size()) || m_body[index] != literal) {
    throw WindowException(
        "Failed at CSSParser.literal(). Literal comparator failed.");
  }
  ++index;
}

std::pair<std::string, std::string_view> CSSParser::pair() {
  std::string property = std::string(word());
  whiteSpace();
  literal(':');
  whiteSpace();
  std::string_view value = word();
  hlp::casefold(property);
  return {property, value};
}

//--WARNING: Get rid of try-catch during testings
std::unordered_map<std::string, std::string_view> CSSParser::body() {
  std::unordered_map<std::string, std::string_view> pairs{};
  while (index < m_body.size()) {
    try {
      auto [property, value] = pair();
      pairs[property] = value;
      whiteSpace();
      literal(';');
      whiteSpace();
    } catch (WindowException exception) {
      auto why = ignore_until(";");
      if (why == ';') {
        literal(';');
        whiteSpace();
      } else
        break;
    }
  }
  return pairs;
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

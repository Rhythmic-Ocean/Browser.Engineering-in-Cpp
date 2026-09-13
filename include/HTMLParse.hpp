#include "helpers.hpp"
#include <array>

class HTMLParse {
  /*
   *--NOTE: std::string_view does not own these strings, but it's usually only
   * dangerous when they live in the stack, here it's in the compiled binary so
   * doesn't matter much since it's not a temporary
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

public:
  HTMLParse(std::string &body);
  Item *parse();
};

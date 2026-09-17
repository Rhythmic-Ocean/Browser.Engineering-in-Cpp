#include "helpers.hpp"
#include "url.hpp"
#include "window.hpp"
#include <memory>

// browser own the nodes too
class Browser {
public:
  static constexpr float WIDTH = 800.0f;
  static constexpr float HEIGHT = 600.0f;

private:
  std::string m_title;
  Window m_window{"Browser", 800, 600};
  std::unique_ptr<Item> m_rootNode{};
  std::unique_ptr<Layout> m_document;
  static std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine;

  void load_engine();

public:
  Browser(std::string &title) : m_title{title} {};
  void load(URL &url);
};

#include "helpers.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

class Window {
  enum text { BOLD, ITALICS, BOLD_ITALICS, REGULAR } text_style;
  text style = REGULAR;
  static constexpr float DEFAULT_MARGIN = 20.0f;
  std::string m_title{};
  int m_width{};
  int m_height{};
  float scroll_y{0.0f};
  float max_y{0.0f};
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window{
      nullptr, &SDL_DestroyWindow};
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer{
      nullptr, &SDL_DestroyRenderer};
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine;
  std::unique_ptr<TTF_Font, FontDeleter> m_font;
  std::unique_ptr<TTF_Font, FontDeleter> m_bold;
  std::unique_ptr<TTF_Font, FontDeleter> m_italics;
  std::unique_ptr<TTF_Font, FontDeleter> m_boldItalics;

  std::vector<DisplayItem> m_items{};
  void init();
  void load_media();
  void load_engine();
  void get_font(std::string &str);
  void process_layout(std::vector<Item> &tokens);
  TTF_Font *choose_font();

  DisplayItem make_display(std::string &word);

public:
  bool is_Running{true};
  Window(const std::string &title, int width, int height);

  SDL_Renderer *getRenderer() const { return m_renderer.get(); }
  SDL_Window *getWindow() const { return m_window.get(); }
  TTF_TextEngine *getTextEngine() const { return m_engine.get(); }
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&other) = default;
  Window &operator=(Window &&other) = default;
  ~Window() = default;

  void start_event();
  static TTF_TextEngine *get_engine();
  void lex(const std::string &body);
  friend void calculate_position(Window &window);
  void draw_text();
};

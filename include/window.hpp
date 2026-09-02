#include "helpers.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

class Window {
  static constexpr float DEFAULT_MARGIN = 20.0f;
  std::string m_title{};
  int m_width{};
  int m_height{};
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window{
      nullptr, &SDL_DestroyWindow};
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer{
      nullptr, &SDL_DestroyRenderer};
  std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> m_font{nullptr,
                                                             &TTF_CloseFont};
  std::unique_ptr<TTF_TextEngine, EngineDeleter> m_engine;

  std::vector<DisplayItem> m_items{};

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
  ~Window() {
    m_font.reset();
    m_renderer.reset();
    m_window.reset();
    TTF_Quit();
    SDL_Quit();
  }

  void start_event();
  SDL_Texture *create_texture(std::string &load);
  void clear();
  static TTF_TextEngine *get_engine();
  void presentTexture(SDL_Texture *texture);
  void lex(const std::string &body);

  friend void calculate_position(Window &window);
  void draw_text();
};

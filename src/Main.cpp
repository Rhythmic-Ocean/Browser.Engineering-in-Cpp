
#include "Browser.hpp"
#include "url.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  std::string url_str{"http://127.0.0.1:8000/"};
  // std::string url_str{"https://browser.engineering/text.html"};
  URL url{url_str};
  { // scope guards so SDL windows and rednerer are destroyed before we quit SDL
    // and TTF
    browser::Browser b{"Browser"};
    b.new_tab(std::move(url));
    while (b.is_Running) {
      b.start_event();
      b.draw();
    }
  }
  TTF_Quit();
  SDL_Quit();
  return 0;
}

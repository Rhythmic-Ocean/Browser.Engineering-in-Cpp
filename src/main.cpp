
#include "Browser.hpp"
#include "HTMLParse.hpp"
#include "url.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  std::string url_str{};
  std::getline(std::cin, url_str);
  // std::string url_str{"https://browser.engineering/text.html"};
  URL url{url_str};
  { // scope guards so SDL windows and rednerer are destroyed before we quit SDL
    // and TTF
    Browser b{"Browser", 800, 600};
    b.load(url);
  }
  TTF_Quit();
  SDL_Quit();
  return 0;
}

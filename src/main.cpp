
#include "url.hpp"
#include "window.hpp"
#include <SDL3/SDL_render.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void load(URL &url) {
  std::string response = url.request();
  Window window{"Browser", 800, 600};
  window.lex(response);
  while (window.is_Running) {
    window.start_event();
    window.draw_text();
  }
}

int main() {
  std::string url_str{};
  std::getline(std::cin, url_str);
  // std::string url_str{
  //     "https://browser.engineering/examples/example1-simple.html"};
  URL url{url_str};
  load(url);
  return 0;
}

## An attempt at implementing the book [Web Browser Engineering](https://browser.engineering/) **_by Pavel Panchekha & Chris Harrelson_** in C++20

The original book's implemented in python.

### Current Status (Latest Sept 2)

#### Chapter 2 - Almost there, just the scrolling functionality is left (Sept 2 2026)

- Used SDL3 for GUI purposes, **_[Mike Shah](https://www.youtube.com/watch?v=kyD5H6w1x-o&list=PLvv0ScY6vfd-RZSmGbLkZvkgec6lJ0BfX&index=1)'s tutorial helped a lot! Also [TTF_TextEngine](https://github.com/libsdl-org/SDL_ttf/blob/055f4bbcc9882320c9ec9ef5329875ed6a8b98bd/docs/hello-textengine.c) was good!_**
  [Chp-2-Mid](assets/image/chp-2-mid.png)

#### Chapter 1 completed (Date: Sept 1 2026)

- Raw POSIX socket networking with <sys/socket.h> _Bunch of help from Chp-11 (Network Programming) on [CS:APP](https://csapp.cs.cmu.edu/)_
- TLS 1.2/1.3 implemented through <openssl/libssl> _Basically C++ implementation of [this](https://docs.openssl.org/master/man7/ossl-guide-tls-client-block/#creating-the-socket-and-bio)_
- HTTP/1.1 headings are configured

[Chp-1](assets/image/chp-1.png)

### Requirements for anybody wanting to run it

- CMake > 3.8
- C++20 capable compiler
- OpenSSL 3.5.7
- SDL3 and SDL3_ttf

  ```
  git clone https://github.com/Rhythmic-Ocean/Browser.Engineering-in-Cpp
  cd Browser
  cmake -B build
  cmake --build build
  cd build
  ./Browser https://browser.engineering/examples/xiyouji.html
  ```

### Next Step: Gonna try and implement Chp-2 with SDL3, let's gooo

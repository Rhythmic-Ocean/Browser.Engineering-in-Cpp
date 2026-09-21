## An attempt at implementing the book [Web Browser Engineering](https://browser.engineering/) **_by Pavel Panchekha & Chris Harrelson_** in C++20

The original book's implemented in python.

> **NOTE:** inter chapter commits are done in the branch chpX-working. It's merged with master once I complete the chapter and the executable's functional!!

### Current Status (Latest Sept 20)

#### Chapter 5 - Completed (Sept 20 2026)

New project structure w/ some description:

```
├── assets //fonts and images (images are only used in this README for now)
│   ├── fonts
│   │   ├── NotoSansSC-Bold.ttf //for CJK characters
│   │   ├── NotoSansSC-Regular.ttf //for CJK characters
│   │   ├── OpenSans-BoldItalic.ttf
│   │   ├── OpenSans-Bold.ttf
│   │   ├── OpenSans-Italic.ttf
│   │   └── OpenSans-Regular.ttf
│   └── image
│   ├── chp-1.png
│   ├── chp-2-mid.png
│   ├── chp3-complete-1.png
│   ├── chp3-complete-2.png
│   ├── chp4-complete-1.png
│   ├── chp4-complete-2.png
│   ├── chp5-complete-1.png
│   └── chp5-complete-2.png
├── CMakeLists.txt
├── include
│   ├── Browser.hpp //created by main, orchestrates the entire rendering process
│   ├── client.hpp //SSL Client to create the TCP connection
│   ├── helpers.hpp //some helper structs, classes and hlp namespace functions
│   ├── HTMLParse.hpp //creates HTML DOM tree and it's main parse() function returns the root
│   ├── layout.hpp //uses the HTML tree to create the Layout Tree which is then rendered thru Browser::draw()
    ├── rio.hpp //robust I/O for communications over TCP, help from CS:APP book. Buffer RIO are deprecated
│   ├── url.hpp //URL and response parsing
├── README.md
└── src
    ├── Browser.cpp
    ├── client.cpp
    ├── fonts.cpp
    ├── helpers.cpp
    ├── HTMLParse.cpp
    ├── layout.cpp
    ├── main.cpp
    ├── rio.cpp
    ├── url.cpp
    └── window.cpp
```

- All rendering are now done thru Layout trees
- More closely follows the book's ways of calculating layout/ text position. But had to remove resizing feature cuz it became really laggy having to redo the entire layout tree every time a resize occured.
- Had a bug in HTMLParse where attributes were not being properly split, now sure how it still worked on the book but I made a seperate attribute_splitter() function for that.
- Rendering [this](https://browser.engineering/layout.html)
  ![Output.txt](assets/image/chp5-complete-1.png)
- Rendering the same page's `<pre>` tags:
  ![Output.txt](assets/image/chp5-complete-2.png)

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

### Next Step: Chapter 6

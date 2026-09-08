## An attempt at implementing the book [Web Browser Engineering](https://browser.engineering/) **_by Pavel Panchekha & Chris Harrelson_** in C++20

The original book's implemented in python.

### Current Status (Latest Sept 7)

#### Chapter 3 - Completed (Sept 7 2026)

modified project's structure a bit, a bit more readable imo

```

├── assets
│   ├── fonts
│   │   ├── NotoSansSC-Bold.ttf
│   │   ├── NotoSansSC-Regular.ttf
│   │   ├── OpenSans-BoldItalic.ttf
│   │   ├── OpenSans-Bold.ttf
│   │   ├── OpenSans-Italic.ttf
│   │   └── OpenSans-Regular.ttf
│   └── image
│       ├── chp-1.png
│       └── chp-2-mid.png
├── CMakeLists.txt
├── include
│   ├── client.hpp
│   ├── fonts.hpp
│   ├── helpers.hpp
│   ├── layout.hpp
│   ├── rio.hpp
│   ├── url.hpp
│   └── window.hpp
├── README.md
└── src
    ├── client.cpp
    ├── fonts.cpp
    ├── helpers.cpp
    ├── layout.cpp
    ├── main.cpp
    ├── rio.cpp
    ├── url.cpp
    └── window.cpp
```

-supports variable size fonts (floating point sizes are degraded to integer sizes)

- no hanging letters/words for variable size fonts
- proper spacing and new line for br and p tags
- Rendering [this](https://browser.engineering/examples/example3-sizes.html)
  ![Chp-3-Completed-1](assets/image/chp-3-completed-1.png)
- Rendering [this](https://browser.engineering/text.html)
  ![Chp-3-Completed-2](assets/image/chp-3-completed-2.png)

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

### Project's structure (subject to change heavily in future)

```

├── assets
│   ├── fonts
│   │   ├── NotoSansSC-Bold.ttf
│   │   ├── NotoSansSC-Regular.ttf
│   │   ├── OpenSans-BoldItalic.ttf
│   │   ├── OpenSans-Bold.ttf
│   │   ├── OpenSans-Italic.ttf
│   │   └── OpenSans-Regular.ttf
│   └── image
│       ├── chp-1.png
│       └── chp-2-mid.png
├── CMakeLists.txt
├── include
│   ├── client.hpp
│   ├── fonts.hpp
│   ├── helpers.hpp
│   ├── layout.hpp
│   ├── rio.hpp
│   ├── url.hpp
│   └── window.hpp
├── README.md
└── src
    ├── client.cpp
    ├── fonts.cpp
    ├── helpers.cpp
    ├── layout.cpp
    ├── main.cpp
    ├── rio.cpp
    ├── url.cpp
    └── window.cpp
```

### Next Step: Chapter 4, parsing HTML DOM

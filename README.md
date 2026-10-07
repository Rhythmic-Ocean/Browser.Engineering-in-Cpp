## An attempt at implementing the book [Web Browser Engineering](https://browser.engineering/) **_by Pavel Panchekha & Chris Harrelson_** in C++20

The original book's implementation is in python.

> **NOTE:** inter chapter commits are done in the branch chpX-working. It's merged with master once I complete the chapter and the executable's functional!!

> This is a learning project, basically my first C++ project after doing learncpp. So probably uses a lot of things that are not best practices.

> I aim to have as little AI generated code as possible in this repo. NOT ANTI-LLM, it's just for me to learn the language and how browsers work.

### Current Status (Latest Sept 27)

#### Chapter 7 - Completed (Oct 7 2026)

- Links can be selected and navigated through
- Multi Tab Support
- Can navigate through history
- Each word has its own Layout now, allowing per word layout modifications
- Successful examples presented below:

- Example usage on [this](https://browser.engineering/)

  ![Output.txt](assets/gifs/chp7-complete-1.png)

### Requirements for anybody wanting to run it

- CMake >= 4.3.0
- C++20 capable compiler
- OpenSSL 3.5.7
- SDL3 and SDL3_ttf

### Steps

- Fork the repo
- Run the following on your linux system:

  ```
  git clone https://github.com/<your-username>/Browser.Engineering-in-Cpp
  cd Browser
  cmake -B build
  cmake --build build
  cd build
  ./Browser https://browser.engineering/examples/xiyouji.html
  ```

### Next Step: Chapter 8

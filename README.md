## An attempt at implementing the book [Web Browser Engineering](https://browser.engineering/) ***by Pavel Panchekha & Chris Harrelson*** in C++20

The original book's implemented in python.

### Current Status
#### Chapter 1 completed!
  - Raw POSIX socket networking with <sys/socket.h> *Bunch of help from Chp-11 (Network Programming) on [CS:APP](https://csapp.cs.cmu.edu/)*
  - TLS 1.2/1.3 implemented through <openssl/libssl> *Basically C++ implementation of [this](https://docs.openssl.org/master/man7/ossl-guide-tls-client-block/#creating-the-socket-and-bio)*
  - HTTP/1.1 headings are configured

### Requirements for anybody wanting to run it:
  - CMake > 3.8
  - C++20 capable compiler
  - OpenSSL 3.5.7

    ```
    git clone https://github.com/Rhythmic-Ocean/Browser.Engineering-in-C-
    cd Browser
    cmake -B build
    cmake --build build
    cd build
    ./Browser
    ```

### Next Step: Gonna try and implement Chp-2 with SDL2, let's gooo!

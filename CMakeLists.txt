cmake_minimum_required(VERSION 3.8...3.31)
project(Browser)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)

set(_SOURCE_DIR
    src/main.cpp
    src/client.cpp
    src/rio.cpp
    src/url.cpp
    src/helpers.cpp)

add_executable(${PROJECT_NAME} ${_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(Browser PRIVATE OpenSSL::SSL OpenSSL::Crypto)

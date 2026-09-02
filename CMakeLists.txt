cmake_minimum_required(VERSION 3.8...3.31)
project(Browser)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

set(_SOURCE_DIR
    src/main.cpp
    src/client.cpp
    src/rio.cpp
    src/url.cpp
    src/helpers.cpp
    src/window.cpp)

add_executable(${PROJECT_NAME} ${_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto SDL3::SDL3 SDL3_ttf::SDL3_ttf)

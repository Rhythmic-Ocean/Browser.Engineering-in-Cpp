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
    src/fonts.cpp
    src/Browser.cpp
    src/InlineLayout.cpp
    src/BlockLayout.cpp
    src/HTMLParser.cpp
    src/CSSParser.cpp
)

add_executable(${PROJECT_NAME} ${_SOURCE_DIR})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
    )
endif()
#
# # Optional clang-tidy static analysis
# find_program(CLANG_TIDY_EXE NAMES clang-tidy)
#
# if(CLANG_TIDY_EXE)
#   set_target_properties(${PROJECT_NAME}
#         PROPERTIES
#             CXX_CLANG_TIDY
#                 "${CLANG_TIDY_EXE};--checks=clang-analyzer-* \
#                                 ,bugprone-*,performance-*"
#     )
# endif()

#include "fonts.hpp"
#include "helpers.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3_ttf/SDL_ttf.h>

void FontCache::init_fontFiles() {
  const std::string fontPath1{"../assets/fonts/OpenSans-Regular.ttf"};
  const std::string fontPath2{"../assets/fonts/OpenSans-Italic.ttf"};
  const std::string fallbackPath1{"../assets/fonts/NotoSansSC-Regular.ttf"};
  const std::string fontPath3{"../assets/fonts/OpenSans-Bold.ttf"};
  const std::string fontPath4{"../assets/fonts/OpenSans-BoldItalic.ttf"};
  const std::string fallbackPath2{"../assets/fonts/NotoSansSC-Bold.ttf"};
  load_fontFiles(fontStyle::REGULAR, fontPath1, fallbackPath1);
  load_fontFiles(fontStyle::ITALICS, fontPath2, fallbackPath1);
  load_fontFiles(fontStyle::BOLD, fontPath3, fallbackPath2);
  load_fontFiles(fontStyle::REGULAR, fontPath1, fallbackPath2);
  return;
}

void FontCache::init_normalFonts() {
  load_font(fontStyle::REGULAR);
  load_font(fontStyle::BOLD);
  load_font(fontStyle::ITALICS);
  load_font(fontStyle::BOLD_ITALICS);
  return;
}

void FontCache::load_fontFiles(fontStyle style, const std::string &primary,
                               const std::string &fallback) {
  FontFile Fontfile1{};
  FontFile Fontfile2{};
  Font myFont1{};
  Font myFont2{};
  size_t fileSize1{};
  size_t fileSize2{};
  auto *file1 = SDL_LoadFile(primary.c_str(), &fileSize1);
  auto *file2 = SDL_LoadFile(fallback.c_str(), &fileSize2);
  if (!file1 || !file2) {
    SDL_Log("Failed to load font file: %s", SDL_GetError());
    throw FontCacheException("Filed to load font file: " + C_SDL_GetStrError() +
                             "\n");
  }
  Fontfile1.reset(new myFile{file1, fileSize1});
  Fontfile2.reset(new myFile{file2, fileSize2});
  fontFile_map[style] = {std::move(Fontfile1), std::move(Fontfile2)};
  return;
}

void FontCache::load_font(fontStyle style) {
  Font myFont1{};
  Font myFont2{};
  auto &[fontFile1, fontFile2] = fontFile_map[style];
  auto *fileStream1 =
      SDL_IOFromConstMem(fontFile1.get()->fontFile, fontFile1.get()->file_size);
  auto *fileStream2 =
      SDL_IOFromConstMem(fontFile2.get()->fontFile, fontFile2.get()->file_size);
  if (!fileStream1 || !fileStream2) {
    SDL_Log("Failed to create IOStream: %s\n", SDL_GetError());
    throw FontCacheException(
        "Failed to create IOStream: " + C_SDL_GetStrError() + "\n");
  }
  auto *font1 = TTF_OpenFontIO(fileStream1, true, globalSize);
  auto *font2 = TTF_OpenFontIO(fileStream1, true, globalSize);
  if (!font1 || !font2) {
    SDL_Log("Failed to open font: %s\n", SDL_GetError());
    throw FontCacheException("Failed to open font: " + C_SDL_GetStrError() +
                             "\n");
  }
  myFont1.reset(font1);
  if (!TTF_AddFallbackFont(font1, font2)) {
    myFont2.reset(font2);
    SDL_Log("Failed to set fallback %s\n", SDL_GetError());
    throw FontCacheException("Failed to set fallback " + C_SDL_GetStrError() +
                             "\n");
  }
  font_vec[static_cast<int>(style)][globalSize] = std::move(myFont1);
  return;
}

FontCache::FontCache() {
  init_fontFiles();
  init_normalFonts();
}

TTF_Font *FontCache::get_font(fontStyle style) {
  int g_size = static_cast<int>(globalSize);
  auto &font_map = font_vec[static_cast<int>(style)];
  if (font_map.find(g_size) == font_map.end()) {
    load_font(style);
  }
  return font_map[g_size].get();
}

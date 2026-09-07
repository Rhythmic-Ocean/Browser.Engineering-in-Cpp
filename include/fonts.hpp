#pragma once

#include "helpers.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>

class FontCache {

  typedef std::unique_ptr<TTF_Font, FontDeleter> Font;
  typedef std::unique_ptr<myFile, FileDeleter> FontFile;

  std::unordered_map<FontStyle, std::pair<FontFile, FontFile>> fontFile_map{};
  std::vector<std::unordered_map<FontSize, Font>> font_vec{
      static_cast<int>(FontStyle::COUNT)};

  void init_fontFiles();
  void init_normalFonts();
  void load_fontFiles(FontStyle style, const std::string &primary,
                      const std::string &fallback);
  void load_font(FontStyle style, FontSize size);

public:
  FontCache();
  void init();
  TTF_Font *get_font(FontStyle style, FontSize size);
  FontCache(FontCache &) = delete;
  FontCache &operator=(FontCache &) = delete;
  FontCache(FontCache &&) = delete;
  FontCache &operator=(FontCache &&) = delete;
  ~FontCache() = default;
};

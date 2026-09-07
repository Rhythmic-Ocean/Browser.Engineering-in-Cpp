#include "helpers.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>

class FontCache {
  enum class fontStyle {
    BOLD,
    ITALICS,
    BOLD_ITALICS,
    REGULAR,
    COUNT
  } globalStyle;
  typedef int fontSize;
  static constexpr float BASE_FONT_SIZE = 16.0f;

  typedef std::unique_ptr<TTF_Font, FontDeleter> Font;
  typedef std::unique_ptr<myFile, FileDeleter> FontFile;

  float globalSize = 16.0f;
  std::unordered_map<fontStyle, std::pair<FontFile, FontFile>> fontFile_map{};
  std::vector<std::unordered_map<fontSize, Font>> font_vec{
      static_cast<int>(fontStyle::COUNT)};

  void init_fontFiles();
  void init_normalFonts();
  void load_fontFiles(fontStyle style, const std::string &primary,
                      const std::string &fallback);
  void load_font(fontStyle style);

public:
  FontCache();
  TTF_Font *get_font(fontStyle style);
};

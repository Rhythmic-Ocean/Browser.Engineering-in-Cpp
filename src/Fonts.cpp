#include "layout.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <exception>
#include <include/core/SkFont.h>
#include <include/core/SkFontStyle.h>
#include <include/core/SkScalar.h>

using namespace Layout;

SkFont FontCache::get_font(SkFontStyle style, SkScalar size,
                           const std::string &family) {
  std::string key = family + "_" + std::to_string(style.weight()) + "_" +
                    std::to_string(style.slant());

  auto it = m_fontMap.find(key);
  if (it != m_fontMap.end()) {
    return SkFont{it->second, size}; // Cache hit!
  }

  // Cache miss: query the OS font manager
  sk_sp<SkTypeface> typeface =
      m_fontMgr->legacyMakeTypeface(family.c_str(), style);
  if (!typeface) {
    typeface = m_fontMgr->legacyMakeTypeface("", SkFontStyle()); // fallback
  }

  m_fontMap[key] = typeface;
  return SkFont{typeface, size};
}

SkFont FontCache::get_font(SkFontStyle style, const std::string &size,
                           const std::string &family) {
  SkScalar sze;
  try {
    sze = std::stof(size.c_str());
  } catch (std::exception &exception) {
    sze = 16.0f;
  }
  return get_font(style, sze, family);
}

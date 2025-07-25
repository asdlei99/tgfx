/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2023 Tencent. All rights reserved.
//
//  Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
//  in compliance with the License. You may obtain a copy of the License at
//
//      https://opensource.org/licenses/BSD-3-Clause
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "SystemFont.h"
#include <algorithm>
#include "tgfx/core/FontStyle.h"

#ifdef _WIN32
#include <dwrite.h>
#include <dwrite_3.h>
#include <array>
#include <locale>
#endif

#pragma clang diagnostic ignored "-Wunused-parameter"

namespace tgfx {
#ifdef _WIN32
struct DWriteFontStyle {
  DWRITE_FONT_WEIGHT weight;
  DWRITE_FONT_STRETCH stretch;
  DWRITE_FONT_STYLE fontStyle;
};

static constexpr std::pair<const char*, DWRITE_FONT_WEIGHT> FontWeightPair[] = {
    {"thin", DWRITE_FONT_WEIGHT_THIN},
    {"hairline", DWRITE_FONT_WEIGHT_THIN},
    {"w1", DWRITE_FONT_WEIGHT_THIN},
    {"extralight", DWRITE_FONT_WEIGHT_EXTRA_LIGHT},
    {"ultralight", DWRITE_FONT_WEIGHT_ULTRA_LIGHT},
    {"w2", DWRITE_FONT_WEIGHT_ULTRA_LIGHT},
    {"light", DWRITE_FONT_WEIGHT_LIGHT},
    {"w3", DWRITE_FONT_WEIGHT_LIGHT},
    {"book", DWRITE_FONT_WEIGHT_SEMI_LIGHT},
    {"normal", DWRITE_FONT_WEIGHT_NORMAL},
    {"plain", DWRITE_FONT_WEIGHT_NORMAL},
    {"roman", DWRITE_FONT_WEIGHT_NORMAL},
    {"standard", DWRITE_FONT_WEIGHT_NORMAL},
    {"w4", DWRITE_FONT_WEIGHT_NORMAL},
    {"r", DWRITE_FONT_WEIGHT_NORMAL},
    {"regular", DWRITE_FONT_WEIGHT_REGULAR},
    {"medium", DWRITE_FONT_WEIGHT_MEDIUM},
    {"w5", DWRITE_FONT_WEIGHT_MEDIUM},
    {"semi", DWRITE_FONT_WEIGHT_SEMI_BOLD},
    {"semibold", DWRITE_FONT_WEIGHT_SEMI_BOLD},
    {"demibold", DWRITE_FONT_WEIGHT_DEMI_BOLD},
    {"w6", DWRITE_FONT_WEIGHT_DEMI_BOLD},
    {"bold", DWRITE_FONT_WEIGHT_BOLD},
    {"w7", DWRITE_FONT_WEIGHT_BOLD},
    {"extra", DWRITE_FONT_WEIGHT_EXTRA_BOLD},
    {"extrabold", DWRITE_FONT_WEIGHT_EXTRA_BOLD},
    {"ultra", DWRITE_FONT_WEIGHT_ULTRA_BOLD},
    {"ultrabold", DWRITE_FONT_WEIGHT_ULTRA_BOLD},
    {"w8", DWRITE_FONT_WEIGHT_ULTRA_BOLD},
    {"black", DWRITE_FONT_WEIGHT_BLACK},
    {"heavy", DWRITE_FONT_WEIGHT_HEAVY},
    {"w9", DWRITE_FONT_WEIGHT_HEAVY},
    {"extrablack", DWRITE_FONT_WEIGHT_EXTRA_BLACK},
    {"ultrablack", DWRITE_FONT_WEIGHT_ULTRA_BLACK},
    {"extraheavy", DWRITE_FONT_WEIGHT_ULTRA_BLACK},
    {"ultraheavy", DWRITE_FONT_WEIGHT_ULTRA_BLACK},
    {"w10", DWRITE_FONT_WEIGHT_ULTRA_BLACK}};

static constexpr std::pair<const char*, DWRITE_FONT_STRETCH> FontWidthPair[] = {
    {"ultracondensed", DWRITE_FONT_STRETCH_ULTRA_CONDENSED},
    {"extracondensed", DWRITE_FONT_STRETCH_EXTRA_CONDENSED},
    {"condensed", DWRITE_FONT_STRETCH_CONDENSED},
    {"narrow", DWRITE_FONT_STRETCH_CONDENSED},
    {"semicondensed", DWRITE_FONT_STRETCH_SEMI_CONDENSED},
    {"normal", DWRITE_FONT_STRETCH_NORMAL},
    {"semiexpanded", DWRITE_FONT_STRETCH_SEMI_EXPANDED},
    {"expanded", DWRITE_FONT_STRETCH_EXPANDED},
    {"extraexpanded", DWRITE_FONT_STRETCH_EXTRA_EXPANDED},
    {"ultraexpanded", DWRITE_FONT_STRETCH_ULTRA_EXPANDED}};

static constexpr std::pair<const char*, DWRITE_FONT_STYLE> FontSlantPair[] = {
    {"upright", DWRITE_FONT_STYLE_NORMAL},
    {"italic", DWRITE_FONT_STYLE_ITALIC},
    {"oblique", DWRITE_FONT_STYLE_OBLIQUE}};

DWriteFontStyle ToDWriteFontStyle(const std::string& fontStyle) {
  std::string key;
  key.resize(fontStyle.length());
  std::transform(fontStyle.begin(), fontStyle.end(), key.begin(), ::tolower);
  DWriteFontStyle dWriteFontStyle{};
  dWriteFontStyle.weight = DWRITE_FONT_WEIGHT_NORMAL;
  dWriteFontStyle.stretch = DWRITE_FONT_STRETCH_NORMAL;
  dWriteFontStyle.fontStyle = DWRITE_FONT_STYLE_NORMAL;

  std::vector<std::string> keys;
  std::string::size_type pos1, pos2;
  pos2 = key.find(' ');
  pos1 = 0;
  while (std::string::npos != pos2) {
    keys.push_back(key.substr(pos1, pos2 - pos1));
    pos1 = pos2 + 1;
    pos2 = key.find(' ', pos1);
  }
  if (pos1 != key.length()) {
    keys.push_back(key.substr(pos1));
  }

  for (auto& styleKey : keys) {
    for (const auto& pair : FontWeightPair) {
      if (pair.first == styleKey) {
        dWriteFontStyle.weight = pair.second;
        break;
      }
    }
    for (const auto& pair : FontWidthPair) {
      if (pair.first == styleKey) {
        dWriteFontStyle.stretch = pair.second;
        break;
      }
    }
    for (const auto& pair : FontSlantPair) {
      if (pair.first == styleKey) {
        dWriteFontStyle.fontStyle = pair.second;
        break;
      }
    }
  }
  return dWriteFontStyle;
}

static constexpr std::array<DWRITE_FONT_WEIGHT, 11> FontWeightMap = {
    DWRITE_FONT_WEIGHT_THIN,      DWRITE_FONT_WEIGHT_THIN,       DWRITE_FONT_WEIGHT_EXTRA_LIGHT,
    DWRITE_FONT_WEIGHT_LIGHT,     DWRITE_FONT_WEIGHT_REGULAR,    DWRITE_FONT_WEIGHT_MEDIUM,
    DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_WEIGHT_BOLD,       DWRITE_FONT_WEIGHT_EXTRA_BOLD,
    DWRITE_FONT_WEIGHT_BLACK,     DWRITE_FONT_WEIGHT_EXTRA_BLACK};

static constexpr std::array<DWRITE_FONT_STRETCH, 9> FontWidthMap = {
    DWRITE_FONT_STRETCH_ULTRA_CONDENSED, DWRITE_FONT_STRETCH_EXTRA_CONDENSED,
    DWRITE_FONT_STRETCH_CONDENSED,       DWRITE_FONT_STRETCH_SEMI_CONDENSED,
    DWRITE_FONT_STRETCH_NORMAL,          DWRITE_FONT_STRETCH_SEMI_EXPANDED,
    DWRITE_FONT_STRETCH_EXPANDED,        DWRITE_FONT_STRETCH_EXTRA_EXPANDED,
    DWRITE_FONT_STRETCH_ULTRA_EXPANDED};

static constexpr std::array<DWRITE_FONT_STYLE, 3> FontSlantMap = {
    DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STYLE_ITALIC, DWRITE_FONT_STYLE_OBLIQUE};

DWriteFontStyle ToDWriteFontStyle(FontStyle fontStyle) {
  DWriteFontStyle dWriteFontStyle{};
  dWriteFontStyle.weight = FontWeightMap[static_cast<size_t>(fontStyle.weight())];
  dWriteFontStyle.stretch = FontWidthMap[static_cast<size_t>(fontStyle.width())];
  dWriteFontStyle.fontStyle = FontSlantMap[static_cast<size_t>(fontStyle.slant())];
  return dWriteFontStyle;
}

std::wstring ToWstring(const std::string& input) {
  std::wstring result;
  int len = MultiByteToWideChar(CP_ACP, 0, input.c_str(), input.size(), nullptr, 0);
  WCHAR* buffer = new (std::nothrow) wchar_t[len + 1];
  if (buffer == nullptr) {
    return result;
  }
  MultiByteToWideChar(CP_ACP, 0, input.c_str(), input.size(), buffer, len);
  buffer[len] = '\0';
  result.append(buffer);
  delete[] buffer;
  return result;
}

std::string ToString(const std::wstring& wstring) {
  std::string result;
  int len =
      WideCharToMultiByte(CP_ACP, 0, wstring.c_str(), wstring.size(), nullptr, 0, nullptr, nullptr);
  char* buffer = new (std::nothrow) char[len + 1];
  if (buffer == nullptr) {
    return result;
  }
  WideCharToMultiByte(CP_ACP, 0, wstring.c_str(), wstring.size(), buffer, len, nullptr, nullptr);
  buffer[len] = '\0';
  result.append(buffer);
  delete[] buffer;
  return result;
}

template <class T>
void SafeRelease(T** ppT) {
  if (*ppT) {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

std::shared_ptr<Typeface> MakeFromFontName(const std::string& fontFamily,
                                           const DWriteFontStyle& fontStyle) {
  if (fontFamily.empty()) {
    return nullptr;
  }
  std::shared_ptr<Typeface> typeface = nullptr;
  IDWriteFactory3* writeFactory = nullptr;
  HRESULT hResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                        reinterpret_cast<IUnknown**>(&writeFactory));
  IDWriteFontSet* fontSet = nullptr;
  if (SUCCEEDED(hResult)) {
    hResult = writeFactory->GetSystemFontSet(&fontSet);
  }
  if (SUCCEEDED(hResult)) {
    hResult = fontSet->GetMatchingFonts(ToWstring(fontFamily).c_str(), fontStyle.weight,
                                        fontStyle.stretch, fontStyle.fontStyle, &fontSet);
  }
  UINT32 fontCount = 0;
  if (SUCCEEDED(hResult)) {
    fontCount = fontSet->GetFontCount();
  }
  if (fontCount > 0) {
    IDWriteFontFaceReference* fontFaceReference = nullptr;
    if (SUCCEEDED(hResult)) {
      hResult = fontSet->GetFontFaceReference(0, &fontFaceReference);
    }
    IDWriteFontFile* fontFile = nullptr;
    if (SUCCEEDED(hResult)) {
      hResult = fontFaceReference->GetFontFile(&fontFile);
    }
    IDWriteLocalFontFileLoader* fontFileLoader = nullptr;
    if (SUCCEEDED(hResult)) {
      hResult = fontFile->GetLoader(reinterpret_cast<IDWriteFontFileLoader**>(&fontFileLoader));
    }

    const void* referenceKey;
    UINT32 fontFileReferenceKeySize = 0;
    if (SUCCEEDED(hResult)) {
      hResult = fontFile->GetReferenceKey(&referenceKey, &fontFileReferenceKeySize);
    }
    UINT32 filePathLength = 100;
    WCHAR* wFilePath = new (std::nothrow) wchar_t[filePathLength];
    if (SUCCEEDED(hResult)) {
      hResult = fontFileLoader->GetFilePathFromKey(referenceKey, fontFileReferenceKeySize,
                                                   wFilePath, filePathLength);
    }
    typeface = Typeface::MakeFromPath(ToString(wFilePath), 0);

    SafeRelease(&fontFileLoader);
    SafeRelease(&fontFile);
    SafeRelease(&fontFaceReference);
    delete[] wFilePath;
  }
  SafeRelease(&fontSet);
  SafeRelease(&writeFactory);
  return typeface;
}
#endif

std::shared_ptr<Typeface> SystemFont::MakeFromName(const std::string& fontFamily,
                                                   const std::string& fontStyle) {
#ifdef _WIN32
  auto dWriteFontStyle = ToDWriteFontStyle(fontStyle);
  return MakeFromFontName(fontFamily, dWriteFontStyle);
#endif
  return nullptr;
}

std::shared_ptr<Typeface> SystemFont::MakeFromName(const std::string& fontFamily,
                                                   FontStyle fontStyle) {
#ifdef _WIN32
  auto dWriteFontStyle = ToDWriteFontStyle(fontStyle);
  return MakeFromFontName(fontFamily, dWriteFontStyle);
#endif
  return nullptr;
}

}  // namespace tgfx

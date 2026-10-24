#include "core/Resources.hpp"
#include "core/SystemFonts.hpp"

#include <algorithm>
#include <cctype>

namespace margelo::nitro::nitropdf {

namespace {

constexpr const char* kFontDir = "/system/fonts/";

bool tryRead(const std::string& file, std::vector<uint8_t>& out) {
  try {
    out = readFile(kFontDir + file);
    return !out.empty();
  } catch (const std::exception&) {
    return false;
  }
}

std::string styleName(bool bold, bool italic) {
  if (bold) return italic ? "BoldItalic" : "Bold";
  return italic ? "Italic" : "Regular";
}

} // namespace

std::vector<uint8_t> loadSystemFont(const std::string& family, bool bold, bool italic) {
  std::string name = family;
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });

  std::vector<uint8_t> bytes;
  if (name == "monospace" || name == "ui-monospace") {
    for (const char* file : {"DroidSansMono.ttf", "CutiveMono.ttf"}) {
      if (tryRead(file, bytes)) return bytes;
    }
    return {};
  }

  std::vector<std::string> bases;
  if (name == "sans-serif" || name == "system-ui" || name == "-apple-system" || name == "blinkmacsystemfont" ||
      name == "ui-sans-serif" || name == "roboto") {
    bases = {"Roboto", "RobotoStatic", "NotoSans", "DroidSans"};
  } else if (name == "serif" || name == "ui-serif") {
    bases = {"NotoSerif", "DroidSerif"};
  } else {
    // e.g. "Noto Sans Mono" -> NotoSansMono-Regular.ttf
    std::string base;
    for (char c : family) {
      if (c != ' ') base += c;
    }
    if (base.empty() || base.find('/') != std::string::npos || base.find("..") != std::string::npos) return {};
    bases = {base};
  }

  // Stay within the first available family; FontManager synthesizes missing bold/italic.
  for (const std::string& base : bases) {
    if ((bold || italic) && tryRead(base + "-" + styleName(bold, italic) + ".ttf", bytes)) return bytes;
    if (tryRead(base + "-Regular.ttf", bytes) || tryRead(base + ".ttf", bytes)) return bytes;
  }
  return {};
}

} // namespace margelo::nitro::nitropdf

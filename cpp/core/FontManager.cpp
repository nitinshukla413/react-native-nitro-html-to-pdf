#include "FontManager.hpp"

#include "Resources.hpp"
#include "SystemFonts.hpp"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace margelo::nitro::nitropdf {

namespace {

std::string lowercase(std::string s) {
  for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::vector<std::string> splitFamilies(const std::string& list) {
  std::vector<std::string> families;
  std::string current;
  char quote = 0;
  auto flush = [&]() {
    size_t b = current.find_first_not_of(" \t\r\n");
    size_t e = current.find_last_not_of(" \t\r\n");
    if (b != std::string::npos) families.push_back(current.substr(b, e - b + 1));
    current.clear();
  };
  for (char c : list) {
    if (quote) {
      if (c == quote) quote = 0;
      else current += c;
    } else if (c == '"' || c == '\'') {
      quote = c;
    } else if (c == ',') {
      flush();
    } else {
      current += c;
    }
  }
  flush();
  return families;
}

} // namespace

FontManager& FontManager::shared() {
  static FontManager instance;
  return instance;
}

void FontManager::registerFont(const std::string& family, const std::string& path, int weight, bool italic) {
  auto font = TrueTypeFont::parse(readFile(pathFromFileUrl(path)));
  std::lock_guard<std::mutex> lock(mutex_);
  registered_.push_back({lowercase(family), weight, italic, std::move(font)});
  resolvedCache_.clear();
}

ResolvedFont FontManager::resolve(const std::string& familyList, int weight, bool italic) {
  std::string key = familyList + '|' + std::to_string(weight) + (italic ? "|i" : "|n");
  std::lock_guard<std::mutex> lock(mutex_);
  if (auto it = resolvedCache_.find(key); it != resolvedCache_.end()) return it->second;

  ResolvedFont result;
  bool found = false;
  for (const std::string& family : splitFamilies(familyList)) {
    if ((found = resolveFamily(family, weight, italic, result))) break;
  }
  if (!found && !resolveFamily("sans-serif", weight, italic, result)) {
    throw std::runtime_error("No usable font found for font-family: " + familyList);
  }
  resolvedCache_[key] = result;
  return result;
}

bool FontManager::resolveFamily(const std::string& family, int weight, bool italic, ResolvedFont& out) {
  bool wantBold = weight >= 600;
  std::string key = lowercase(family);

  const Registered* best = nullptr;
  int bestScore = 0;
  for (const Registered& r : registered_) {
    if (r.family != key) continue;
    int score = std::abs(r.weight - weight) + (r.italic != italic ? 1000 : 0);
    if (!best || score < bestScore) {
      best = &r;
      bestScore = score;
    }
  }
  if (best) {
    out.font = best->font;
    out.syntheticBold = wantBold && best->weight < 600;
    out.syntheticItalic = italic && !best->italic;
    return true;
  }

  auto font = systemFont(family, wantBold, italic);
  if (!font) return false;
  out.font = font;
  out.syntheticBold = wantBold && !font->metrics().bold;
  out.syntheticItalic = italic && !font->metrics().italic;
  return true;
}

std::shared_ptr<const TrueTypeFont> FontManager::systemFont(const std::string& family, bool bold, bool italic) {
  std::string key = lowercase(family) + (bold ? "|b" : "|r") + (italic ? "i" : "n");
  if (auto it = systemCache_.find(key); it != systemCache_.end()) return it->second;

  std::shared_ptr<const TrueTypeFont> font;
  std::vector<uint8_t> bytes = loadSystemFont(family, bold, italic);
  if (!bytes.empty()) {
    try {
      font = TrueTypeFont::parse(std::move(bytes));
    } catch (const std::exception&) {
      font = nullptr;
    }
  }
  systemCache_[key] = font;
  return font;
}

} // namespace margelo::nitro::nitropdf

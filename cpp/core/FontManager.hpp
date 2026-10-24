#pragma once

#include "TrueTypeFont.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace margelo::nitro::nitropdf {

struct ResolvedFont {
  std::shared_ptr<const TrueTypeFont> font;
  bool syntheticBold = false;
  bool syntheticItalic = false;
};

/** Process-wide font registry and cache, shared by all render threads. */
class FontManager {
public:
  static FontManager& shared();

  void registerFont(const std::string& family, const std::string& path, int weight, bool italic);

  /** `familyList` is a CSS `font-family` value, e.g. `"Inter", Arial, sans-serif`. */
  ResolvedFont resolve(const std::string& familyList, int weight, bool italic);

private:
  struct Registered {
    std::string family;
    int weight;
    bool italic;
    std::shared_ptr<const TrueTypeFont> font;
  };

  bool resolveFamily(const std::string& family, int weight, bool italic, ResolvedFont& out);
  std::shared_ptr<const TrueTypeFont> systemFont(const std::string& family, bool bold, bool italic);

  std::mutex mutex_;
  std::vector<Registered> registered_;
  std::unordered_map<std::string, std::shared_ptr<const TrueTypeFont>> systemCache_;
  std::unordered_map<std::string, ResolvedFont> resolvedCache_;
};

} // namespace margelo::nitro::nitropdf

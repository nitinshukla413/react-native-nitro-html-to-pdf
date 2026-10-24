#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace margelo::nitro::nitropdf {

/** Immutable, thread-safe view of a TrueType (glyf) or OpenType (CFF) font program. */
class TrueTypeFont {
public:
  struct Table {
    uint32_t tag;
    std::vector<uint8_t> data;
  };

  /** All values in font units, except `italicAngle` (degrees). */
  struct Metrics {
    int unitsPerEm = 1000;
    int ascender = 0;
    int descender = 0;
    int lineGap = 0;
    int capHeight = 0;
    int xHeight = 0;
    int xMin = 0, yMin = 0, xMax = 0, yMax = 0;
    double italicAngle = 0;
    int underlinePosition = 0;
    int underlineThickness = 0;
    int strikeoutPosition = 0;
    bool bold = false;
    bool italic = false;
    bool fixedPitch = false;
  };

  /** Throws if `data` is not a usable font. Collections (TTC) use their first face. */
  static std::shared_ptr<const TrueTypeFont> parse(std::vector<uint8_t> data);
  static std::vector<uint8_t> assemble(std::vector<Table> tables);

  uint16_t glyphId(uint32_t codepoint) const;
  uint16_t advanceWidth(uint16_t glyph) const { return glyph < advances_.size() ? advances_[glyph] : 0; }
  uint16_t numGlyphs() const { return numGlyphs_; }
  bool isCff() const { return isCff_; }
  const Metrics& metrics() const { return metrics_; }
  const std::string& postScriptName() const { return postScriptName_; }

  /** TrueType keeps only `glyphs` (glyph ids unchanged); CFF fonts are embedded whole. */
  std::vector<uint8_t> embeddable(const std::vector<bool>& glyphs) const;

private:
  struct Range {
    size_t offset = 0;
    size_t length = 0;
  };

  TrueTypeFont() = default;
  const Range* table(uint32_t tag) const;
  uint16_t lookupCmap(uint32_t codepoint) const;
  size_t glyphOffset(uint16_t glyph) const;
  std::vector<uint8_t> copyTable(uint32_t tag) const;

  std::vector<uint8_t> data_;
  std::unordered_map<uint32_t, Range> tables_;
  Metrics metrics_;
  std::string postScriptName_;
  std::vector<uint16_t> advances_;
  std::array<uint16_t, 256> latin1_{};
  uint16_t numGlyphs_ = 0;
  bool isCff_ = false;
  bool longLoca_ = false;
  size_t cmapOffset_ = 0;
  uint16_t cmapFormat_ = 0;
  bool symbolCmap_ = false;
};

} // namespace margelo::nitro::nitropdf

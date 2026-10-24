#pragma once

#include "ImageDecoder.hpp"
#include "TrueTypeFont.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace margelo::nitro::nitropdf {

/** Appends `value` with up to 3 decimals, independent of the C locale. */
void appendNumber(std::string& out, double value);

class PdfWriter {
public:
  /** Registers the font on first use; its resource name is `/F<index + 1>`. */
  size_t fontIndex(const std::shared_ptr<const TrueTypeFont>& font);
  void useGlyph(size_t fontIndex, uint16_t glyph, uint32_t codepoint);
  std::string imageName(const std::shared_ptr<const PdfImage>& image);
  std::string alphaStateName(uint8_t alpha);
  /** `dictionaryBody` is the shading dictionary without `<< >>`. */
  std::string shadingName(std::string dictionaryBody);
  void setTitle(const std::string& utf8Title) { title_ = utf8Title; }
  void addPage(double widthPt, double heightPt, const std::string& content);

  size_t pageCount() const { return pages_.size(); }
  std::vector<uint8_t> finish();

private:
  struct FontEntry {
    std::shared_ptr<const TrueTypeFont> font;
    /** Unicode value + 1 per glyph id (0 = unused). */
    std::vector<uint32_t> glyphToUnicode;
  };
  struct Page {
    double width;
    double height;
    std::vector<uint8_t> content;
  };

  std::vector<FontEntry> fonts_;
  std::unordered_map<const TrueTypeFont*, size_t> fontLookup_;
  std::vector<std::shared_ptr<const PdfImage>> images_;
  std::unordered_map<const PdfImage*, size_t> imageLookup_;
  std::vector<uint8_t> alphaStates_;
  std::vector<std::string> shadings_;
  std::vector<Page> pages_;
  std::string title_;
};

} // namespace margelo::nitro::nitropdf

#include "../cpp/core/SystemFonts.hpp"
#include "../cpp/core/TrueTypeFont.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>

#include <algorithm>
#include <cctype>

namespace margelo::nitro::nitropdf {

namespace {

std::vector<std::string> candidateFamilies(const std::string& family) {
  std::string name = family;
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
  if (name == "sans-serif" || name == "system-ui" || name == "-apple-system" || name == "blinkmacsystemfont" ||
      name == "ui-sans-serif") {
    return {"Helvetica", "Arial"};
  }
  if (name == "serif" || name == "ui-serif") return {"Times New Roman", "Times"};
  if (name == "monospace" || name == "ui-monospace") return {"Menlo", "Courier New", "Courier"};
  return {family};
}

/** Returns nullptr unless CoreText has this exact family; it would otherwise substitute another font. */
CTFontRef createFont(const std::string& family, bool bold, bool italic) {
  CFStringRef name = CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(family.data()),
                                             CFIndex(family.size()), kCFStringEncodingUTF8, false);
  if (!name) return nullptr;

  uint32_t traits = (bold ? kCTFontTraitBold : 0) | (italic ? kCTFontTraitItalic : 0);
  CFNumberRef traitValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &traits);
  const void* traitKeys[] = {kCTFontSymbolicTrait};
  const void* traitValues[] = {traitValue};
  CFDictionaryRef traitDict = CFDictionaryCreate(kCFAllocatorDefault, traitKeys, traitValues, 1,
                                                 &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  const void* keys[] = {kCTFontFamilyNameAttribute, kCTFontTraitsAttribute};
  const void* values[] = {name, traitDict};
  CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, keys, values, traits ? 2 : 1,
                                                  &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(attributes);
  CTFontDescriptorRef match = CTFontDescriptorCreateMatchingFontDescriptor(descriptor, nullptr);

  CTFontRef font = nullptr;
  if (match) {
    auto matchedFamily = static_cast<CFStringRef>(CTFontDescriptorCopyAttribute(match, kCTFontFamilyNameAttribute));
    if (matchedFamily && CFStringCompare(matchedFamily, name, kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
      font = CTFontCreateWithFontDescriptor(match, 12, nullptr);
    }
    if (matchedFamily) CFRelease(matchedFamily);
    CFRelease(match);
  }
  CFRelease(descriptor);
  CFRelease(attributes);
  CFRelease(traitDict);
  CFRelease(traitValue);
  CFRelease(name);
  return font;
}

std::vector<uint8_t> fontProgram(CTFontRef font) {
  // System font files aren't readable from the app sandbox, so rebuild the font from its tables.
  static const CTFontTableTag tags[] = {
      kCTFontTableHead, kCTFontTableHhea, kCTFontTableHmtx, kCTFontTableMaxp, kCTFontTableCmap,
      kCTFontTableGlyf, kCTFontTableLoca, kCTFontTableCFF,  kCTFontTableOS2,  kCTFontTablePost,
      kCTFontTableName, kCTFontTableCvt,  kCTFontTableFpgm, kCTFontTablePrep,
  };
  std::vector<TrueTypeFont::Table> tables;
  for (CTFontTableTag tag : tags) {
    CFDataRef data = CTFontCopyTable(font, tag, kCTFontTableOptionNoOptions);
    if (!data) continue;
    const UInt8* bytes = CFDataGetBytePtr(data);
    tables.push_back({tag, std::vector<uint8_t>(bytes, bytes + CFDataGetLength(data))});
    CFRelease(data);
  }
  if (tables.empty()) return {};
  return TrueTypeFont::assemble(std::move(tables));
}

} // namespace

std::vector<uint8_t> loadSystemFont(const std::string& family, bool bold, bool italic) {
  for (const std::string& candidate : candidateFamilies(family)) {
    CTFontRef font = createFont(candidate, bold, italic);
    if (!font && (bold || italic)) font = createFont(candidate, false, false);
    if (!font) continue;
    std::vector<uint8_t> program = fontProgram(font);
    CFRelease(font);
    if (!program.empty()) return program;
  }
  return {};
}

} // namespace margelo::nitro::nitropdf

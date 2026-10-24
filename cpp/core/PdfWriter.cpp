#include "PdfWriter.hpp"

#include "Utf8.hpp"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <zlib.h>

namespace margelo::nitro::nitropdf {

namespace {

std::vector<uint8_t> deflate(const uint8_t* data, size_t size) {
  uLongf length = compressBound(uLong(size));
  std::vector<uint8_t> out(length);
  if (compress2(out.data(), &length, data, uLong(size), Z_DEFAULT_COMPRESSION) != Z_OK) {
    throw std::runtime_error("Failed to compress PDF stream");
  }
  out.resize(length);
  return out;
}

std::string ref(size_t id) {
  return std::to_string(id) + " 0 R";
}

class Output {
public:
  std::vector<uint8_t> bytes;
  std::vector<size_t> offsets;

  void append(std::string_view s) { bytes.insert(bytes.end(), s.begin(), s.end()); }
  void append(const std::vector<uint8_t>& data) { bytes.insert(bytes.end(), data.begin(), data.end()); }

  void begin(size_t id) {
    if (offsets.size() <= id) offsets.resize(id + 1, 0);
    offsets[id] = bytes.size();
    append(std::to_string(id));
    append(" 0 obj\n");
  }

  void object(size_t id, const std::string& body) {
    begin(id);
    append(body);
    append("\nendobj\n");
  }

  void stream(size_t id, const std::string& dictionary, const std::vector<uint8_t>& data) {
    begin(id);
    append("<<");
    append(dictionary);
    append(" /Length ");
    append(std::to_string(data.size()));
    append(" >>\nstream\n");
    append(data);
    append("\nendstream\nendobj\n");
  }
};

std::string subsetTag(size_t index, const std::string& name) {
  uint32_t hash = 2166136261u ^ uint32_t(index * 2654435761u);
  for (char c : name) hash = (hash ^ uint8_t(c)) * 16777619u;
  std::string tag;
  for (int i = 0; i < 6; i++) {
    tag += char('A' + hash % 26);
    hash /= 26;
  }
  return tag;
}

} // namespace

void appendNumber(std::string& out, double value) {
  if (!std::isfinite(value)) value = 0;
  long long scaled = std::llround(value * 1000.0);
  if (scaled < 0) {
    out += '-';
    scaled = -scaled;
  }
  out += std::to_string(scaled / 1000);
  int fraction = int(scaled % 1000);
  if (fraction == 0) return;
  char digits[4] = {'.', char('0' + fraction / 100), char('0' + (fraction / 10) % 10), char('0' + fraction % 10)};
  size_t length = 4;
  while (digits[length - 1] == '0') length--;
  out.append(digits, length);
}

size_t PdfWriter::fontIndex(const std::shared_ptr<const TrueTypeFont>& font) {
  auto it = fontLookup_.find(font.get());
  if (it != fontLookup_.end()) return it->second;
  size_t index = fonts_.size();
  fonts_.push_back({font, std::vector<uint32_t>(font->numGlyphs(), 0)});
  fontLookup_[font.get()] = index;
  return index;
}

void PdfWriter::useGlyph(size_t fontIndex, uint16_t glyph, uint32_t codepoint) {
  auto& map = fonts_[fontIndex].glyphToUnicode;
  if (glyph < map.size() && map[glyph] == 0) map[glyph] = codepoint + 1;
}

std::string PdfWriter::imageName(const std::shared_ptr<const PdfImage>& image) {
  auto it = imageLookup_.find(image.get());
  size_t index;
  if (it != imageLookup_.end()) {
    index = it->second;
  } else {
    index = images_.size();
    images_.push_back(image);
    imageLookup_[image.get()] = index;
  }
  return "Im" + std::to_string(index + 1);
}

std::string PdfWriter::alphaStateName(uint8_t alpha) {
  size_t index = 0;
  while (index < alphaStates_.size() && alphaStates_[index] != alpha) index++;
  if (index == alphaStates_.size()) alphaStates_.push_back(alpha);
  return "GS" + std::to_string(index + 1);
}

std::string PdfWriter::shadingName(std::string dictionaryBody) {
  shadings_.push_back(std::move(dictionaryBody));
  return "Sh" + std::to_string(shadings_.size());
}

void PdfWriter::addPage(double widthPt, double heightPt, const std::string& content) {
  pages_.push_back({widthPt, heightPt, deflate(reinterpret_cast<const uint8_t*>(content.data()), content.size())});
}

std::vector<uint8_t> PdfWriter::finish() {
  Output out;
  out.bytes.reserve(64 * 1024);
  out.append("%PDF-1.7\n%\xE2\xE3\xCF\xD3\n");

  size_t next = 1;
  const size_t catalogId = next++, pagesId = next++, resourcesId = next++, infoId = next++;

  std::string fontResources;
  for (size_t i = 0; i < fonts_.size(); i++) {
    const FontEntry& entry = fonts_[i];
    const TrueTypeFont& font = *entry.font;
    const auto& m = font.metrics();
    const double scale = 1000.0 / m.unitsPerEm;
    size_t type0Id = next++, cidId = next++, descriptorId = next++, fileId = next++, toUnicodeId = next++;

    std::vector<bool> used(entry.glyphToUnicode.size());
    for (size_t g = 0; g < used.size(); g++) used[g] = entry.glyphToUnicode[g] != 0;
    std::vector<uint8_t> program = font.embeddable(used);
    std::string name = subsetTag(i, font.postScriptName()) + "+" + font.postScriptName();

    std::string widths;
    size_t g = 0;
    while (g < used.size()) {
      if (!used[g]) {
        g++;
        continue;
      }
      widths += std::to_string(g) + " [";
      while (g < used.size() && used[g]) {
        widths += std::to_string(std::lround(font.advanceWidth(uint16_t(g)) * scale)) + " ";
        g++;
      }
      widths += "] ";
    }

    std::string cmap =
        "/CIDInit /ProcSet findresource begin\n12 dict begin\nbegincmap\n"
        "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
        "/CMapName /Adobe-Identity-UCS def\n/CMapType 2 def\n"
        "1 begincodespacerange\n<0000> <FFFF>\nendcodespacerange\n";
    std::string block;
    size_t blockCount = 0;
    auto flushBlock = [&]() {
      if (blockCount == 0) return;
      cmap += std::to_string(blockCount) + " beginbfchar\n" + block + "endbfchar\n";
      block.clear();
      blockCount = 0;
    };
    for (size_t gid = 0; gid < entry.glyphToUnicode.size(); gid++) {
      uint32_t value = entry.glyphToUnicode[gid];
      if (value == 0 || gid == 0) continue;
      block += '<';
      appendHex4(block, uint32_t(gid));
      block += "> <";
      appendUtf16Hex(block, value - 1);
      block += ">\n";
      if (++blockCount == 100) flushBlock();
    }
    flushBlock();
    cmap += "endcmap\nCMapName currentdict /CMap defineresource pop\nend\nend\n";

    int flags = 4 | (m.fixedPitch ? 1 : 0) | (m.italic ? 64 : 0);
    std::string descriptor = "<< /Type /FontDescriptor /FontName /" + name + " /Flags " + std::to_string(flags) +
                             " /FontBBox [";
    for (int v : {m.xMin, m.yMin, m.xMax, m.yMax}) descriptor += std::to_string(std::lround(v * scale)) + " ";
    descriptor += "] /ItalicAngle ";
    appendNumber(descriptor, m.italicAngle);
    descriptor += " /Ascent " + std::to_string(std::lround(m.ascender * scale)) + " /Descent " +
                  std::to_string(std::lround(m.descender * scale)) + " /CapHeight " +
                  std::to_string(std::lround(m.capHeight * scale)) + " /StemV 80 " +
                  (font.isCff() ? "/FontFile3 " : "/FontFile2 ") + ref(fileId) + " >>";

    out.object(type0Id, "<< /Type /Font /Subtype /Type0 /BaseFont /" + name +
                            " /Encoding /Identity-H /DescendantFonts [" + ref(cidId) + "] /ToUnicode " +
                            ref(toUnicodeId) + " >>");
    out.object(cidId, std::string("<< /Type /Font /Subtype ") + (font.isCff() ? "/CIDFontType0" : "/CIDFontType2") +
                          " /BaseFont /" + name +
                          " /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >> /FontDescriptor " +
                          ref(descriptorId) + " /W [" + widths + "]" +
                          (font.isCff() ? "" : " /CIDToGIDMap /Identity") + " >>");
    out.object(descriptorId, descriptor);
    std::string fileDict = font.isCff() ? " /Subtype /OpenType" : " /Length1 " + std::to_string(program.size());
    out.stream(fileId, fileDict + " /Filter /FlateDecode", deflate(program.data(), program.size()));
    out.stream(toUnicodeId, " /Filter /FlateDecode",
               deflate(reinterpret_cast<const uint8_t*>(cmap.data()), cmap.size()));
    fontResources += " /F" + std::to_string(i + 1) + " " + ref(type0Id);
  }

  std::string imageResources;
  for (size_t i = 0; i < images_.size(); i++) {
    const PdfImage& image = *images_[i];
    size_t imageId = next++;
    size_t maskId = image.alpha.empty() ? 0 : next++;
    std::string size = " /Width " + std::to_string(image.width) + " /Height " + std::to_string(image.height) +
                       " /BitsPerComponent 8";
    const char* colorSpace = image.components == 1 ? "/DeviceGray" : image.components == 4 ? "/DeviceCMYK" : "/DeviceRGB";
    std::string dict = " /Type /XObject /Subtype /Image" + size + " /ColorSpace " + colorSpace;
    if (maskId) dict += " /SMask " + ref(maskId);
    if (image.isJpeg) {
      dict += " /Filter /DCTDecode";
      if (image.invertedCmyk) dict += " /Decode [1 0 1 0 1 0 1 0]";
      out.stream(imageId, dict, image.data);
    } else {
      out.stream(imageId, dict + " /Filter /FlateDecode", deflate(image.data.data(), image.data.size()));
    }
    if (maskId) {
      out.stream(maskId, " /Type /XObject /Subtype /Image" + size + " /ColorSpace /DeviceGray /Filter /FlateDecode",
                 deflate(image.alpha.data(), image.alpha.size()));
    }
    imageResources += " /Im" + std::to_string(i + 1) + " " + ref(imageId);
  }

  std::string shadingResources;
  for (size_t i = 0; i < shadings_.size(); i++) {
    size_t id = next++;
    out.object(id, "<<" + shadings_[i] + " >>");
    shadingResources += " /Sh" + std::to_string(i + 1) + " " + ref(id);
  }

  std::string resources = "<< /ProcSet [/PDF /Text /ImageB /ImageC]";
  if (!fontResources.empty()) resources += " /Font <<" + fontResources + " >>";
  if (!imageResources.empty()) resources += " /XObject <<" + imageResources + " >>";
  if (!shadingResources.empty()) resources += " /Shading <<" + shadingResources + " >>";
  if (!alphaStates_.empty()) {
    resources += " /ExtGState <<";
    for (size_t i = 0; i < alphaStates_.size(); i++) {
      std::string alpha;
      appendNumber(alpha, alphaStates_[i] / 255.0);
      resources += " /GS" + std::to_string(i + 1) + " << /Type /ExtGState /ca " + alpha + " /CA " + alpha + " >>";
    }
    resources += " >>";
  }
  resources += " >>";
  out.object(resourcesId, resources);

  std::string kids;
  for (const Page& page : pages_) {
    size_t pageId = next++, contentId = next++;
    out.stream(contentId, " /Filter /FlateDecode", page.content);
    std::string box;
    appendNumber(box, page.width);
    box += ' ';
    appendNumber(box, page.height);
    out.object(pageId, "<< /Type /Page /Parent " + ref(pagesId) + " /MediaBox [0 0 " + box + "] /Resources " +
                           ref(resourcesId) + " /Contents " + ref(contentId) + " >>");
    kids += ref(pageId) + " ";
  }
  out.object(pagesId, "<< /Type /Pages /Kids [" + kids + "] /Count " + std::to_string(pages_.size()) + " >>");
  out.object(catalogId, "<< /Type /Catalog /Pages " + ref(pagesId) + " >>");

  std::string info = "<< /Producer (react-native-nitro-html-to-pdf)";
  if (!title_.empty()) {
    info += " /Title <FEFF";
    const char* s = title_.c_str();
    while (*s) appendUtf16Hex(info, decodeUtf8(s));
    info += ">";
  }
  out.object(infoId, info + " >>");

  size_t xref = out.bytes.size();
  out.append("xref\n0 " + std::to_string(next) + "\n0000000000 65535 f \n");
  char line[24];
  for (size_t id = 1; id < next; id++) {
    std::snprintf(line, sizeof(line), "%010zu 00000 n \n", id < out.offsets.size() ? out.offsets[id] : size_t(0));
    out.append(line);
  }
  out.append("trailer\n<< /Size " + std::to_string(next) + " /Root " + ref(catalogId) + " /Info " + ref(infoId) +
             " >>\nstartxref\n" + std::to_string(xref) + "\n%%EOF\n");
  return std::move(out.bytes);
}

} // namespace margelo::nitro::nitropdf

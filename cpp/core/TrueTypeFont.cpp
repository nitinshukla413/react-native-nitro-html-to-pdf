#include "TrueTypeFont.hpp"

#include <algorithm>
#include <stdexcept>

namespace margelo::nitro::nitropdf {

namespace {

constexpr uint32_t makeTag(char a, char b, char c, char d) {
  return (uint32_t(uint8_t(a)) << 24) | (uint32_t(uint8_t(b)) << 16) | (uint32_t(uint8_t(c)) << 8) | uint32_t(uint8_t(d));
}

constexpr uint32_t kHead = makeTag('h', 'e', 'a', 'd');
constexpr uint32_t kHhea = makeTag('h', 'h', 'e', 'a');
constexpr uint32_t kHmtx = makeTag('h', 'm', 't', 'x');
constexpr uint32_t kMaxp = makeTag('m', 'a', 'x', 'p');
constexpr uint32_t kCmap = makeTag('c', 'm', 'a', 'p');
constexpr uint32_t kGlyf = makeTag('g', 'l', 'y', 'f');
constexpr uint32_t kLoca = makeTag('l', 'o', 'c', 'a');
constexpr uint32_t kCff = makeTag('C', 'F', 'F', ' ');
constexpr uint32_t kOs2 = makeTag('O', 'S', '/', '2');
constexpr uint32_t kPost = makeTag('p', 'o', 's', 't');
constexpr uint32_t kName = makeTag('n', 'a', 'm', 'e');
constexpr uint32_t kCvt = makeTag('c', 'v', 't', ' ');
constexpr uint32_t kFpgm = makeTag('f', 'p', 'g', 'm');
constexpr uint32_t kPrep = makeTag('p', 'r', 'e', 'p');

class Reader {
public:
  Reader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

  void check(size_t offset, size_t length) const {
    if (offset > size_ || length > size_ - offset) throw std::runtime_error("Font data is truncated or corrupt");
  }
  uint8_t u8(size_t o) const { check(o, 1); return data_[o]; }
  uint16_t u16(size_t o) const { check(o, 2); return uint16_t((data_[o] << 8) | data_[o + 1]); }
  int16_t s16(size_t o) const { return int16_t(u16(o)); }
  uint32_t u32(size_t o) const {
    check(o, 4);
    return (uint32_t(data_[o]) << 24) | (uint32_t(data_[o + 1]) << 16) | (uint32_t(data_[o + 2]) << 8) | data_[o + 3];
  }

private:
  const uint8_t* data_;
  size_t size_;
};

void putU16(std::vector<uint8_t>& out, size_t o, uint16_t v) {
  out[o] = uint8_t(v >> 8);
  out[o + 1] = uint8_t(v);
}

void putU32(std::vector<uint8_t>& out, size_t o, uint32_t v) {
  out[o] = uint8_t(v >> 24);
  out[o + 1] = uint8_t(v >> 16);
  out[o + 2] = uint8_t(v >> 8);
  out[o + 3] = uint8_t(v);
}

uint32_t checksum(const uint8_t* data, size_t length) {
  uint32_t sum = 0;
  for (size_t i = 0; i < length; i += 4) {
    uint32_t word = 0;
    for (size_t j = 0; j < 4; j++) word = (word << 8) | (i + j < length ? data[i + j] : 0);
    sum += word;
  }
  return sum;
}

std::string sanitizePostScriptName(const std::string& name) {
  std::string out;
  for (char c : name) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') out += c;
    if (out.size() == 63) break;
  }
  return out.empty() ? "Font" : out;
}

} // namespace

const TrueTypeFont::Range* TrueTypeFont::table(uint32_t tag) const {
  auto it = tables_.find(tag);
  return it == tables_.end() ? nullptr : &it->second;
}

std::shared_ptr<const TrueTypeFont> TrueTypeFont::parse(std::vector<uint8_t> bytes) {
  std::shared_ptr<TrueTypeFont> font(new TrueTypeFont());
  font->data_ = std::move(bytes);
  Reader r(font->data_.data(), font->data_.size());

  size_t dir = 0;
  uint32_t version = r.u32(0);
  if (version == makeTag('t', 't', 'c', 'f')) {
    if (r.u32(8) == 0) throw std::runtime_error("Empty font collection");
    dir = r.u32(12);
    version = r.u32(dir);
  }
  if (version != 0x00010000 && version != makeTag('O', 'T', 'T', 'O') && version != makeTag('t', 'r', 'u', 'e')) {
    throw std::runtime_error("Unsupported font format (expected TrueType or OpenType)");
  }

  uint16_t numTables = r.u16(dir + 4);
  for (uint16_t i = 0; i < numTables; i++) {
    size_t rec = dir + 12 + size_t(i) * 16;
    Range range{r.u32(rec + 8), r.u32(rec + 12)};
    r.check(range.offset, range.length);
    font->tables_[r.u32(rec)] = range;
  }

  const Range* head = font->table(kHead);
  const Range* hhea = font->table(kHhea);
  const Range* hmtx = font->table(kHmtx);
  const Range* maxp = font->table(kMaxp);
  const Range* cmap = font->table(kCmap);
  if (!head || !hhea || !hmtx || !maxp || !cmap) throw std::runtime_error("Font is missing required tables");

  font->isCff_ = font->table(kCff) != nullptr;
  if (!font->isCff_ && (!font->table(kGlyf) || !font->table(kLoca))) {
    throw std::runtime_error("Font has no supported outlines (glyf or CFF)");
  }

  Metrics& m = font->metrics_;
  m.unitsPerEm = r.u16(head->offset + 18);
  if (m.unitsPerEm < 16 || m.unitsPerEm > 16384) throw std::runtime_error("Font has an invalid unitsPerEm");
  m.xMin = r.s16(head->offset + 36);
  m.yMin = r.s16(head->offset + 38);
  m.xMax = r.s16(head->offset + 40);
  m.yMax = r.s16(head->offset + 42);
  uint16_t macStyle = r.u16(head->offset + 44);
  font->longLoca_ = r.s16(head->offset + 50) != 0;
  m.bold = (macStyle & 1) != 0;
  m.italic = (macStyle & 2) != 0;

  m.ascender = r.s16(hhea->offset + 4);
  m.descender = r.s16(hhea->offset + 6);
  m.lineGap = r.s16(hhea->offset + 8);
  uint16_t numHMetrics = r.u16(hhea->offset + 34);
  font->numGlyphs_ = r.u16(maxp->offset + 4);
  if (numHMetrics == 0 || font->numGlyphs_ == 0) throw std::runtime_error("Font has no glyph metrics");
  r.check(hmtx->offset, size_t(numHMetrics) * 4);

  font->advances_.resize(font->numGlyphs_);
  for (uint16_t g = 0; g < font->numGlyphs_; g++) {
    uint16_t idx = std::min<uint16_t>(g, numHMetrics - 1);
    font->advances_[g] = r.u16(hmtx->offset + size_t(idx) * 4);
  }

  m.capHeight = int(m.ascender * 0.7);
  m.xHeight = int(m.ascender * 0.5);
  m.strikeoutPosition = m.unitsPerEm / 4;
  if (const Range* os2 = font->table(kOs2); os2 && os2->length >= 78) {
    uint16_t os2Version = r.u16(os2->offset);
    uint16_t weightClass = r.u16(os2->offset + 4);
    m.strikeoutPosition = r.s16(os2->offset + 28);
    uint16_t fsSelection = r.u16(os2->offset + 62);
    m.italic = m.italic || (fsSelection & 1) != 0;
    m.bold = m.bold || (fsSelection & 32) != 0 || weightClass >= 600;
    if (m.ascender == 0 && m.descender == 0) {
      m.ascender = r.s16(os2->offset + 68);
      m.descender = r.s16(os2->offset + 70);
      m.lineGap = r.s16(os2->offset + 72);
    }
    if (os2Version >= 2 && os2->length >= 90) {
      if (int16_t x = r.s16(os2->offset + 86); x > 0) m.xHeight = x;
      if (int16_t c = r.s16(os2->offset + 88); c > 0) m.capHeight = c;
    }
  }
  m.underlinePosition = -m.unitsPerEm / 10;
  m.underlineThickness = m.unitsPerEm / 20;
  if (const Range* post = font->table(kPost); post && post->length >= 16) {
    m.italicAngle = int32_t(r.u32(post->offset + 4)) / 65536.0;
    if (int16_t pos = r.s16(post->offset + 8); pos != 0) m.underlinePosition = pos;
    if (int16_t thick = r.s16(post->offset + 10); thick > 0) m.underlineThickness = thick;
    m.fixedPitch = r.u32(post->offset + 12) != 0;
  }

  if (const Range* name = font->table(kName); name && name->length >= 6) {
    uint16_t count = r.u16(name->offset + 2);
    size_t strings = name->offset + r.u16(name->offset + 4);
    for (uint16_t i = 0; i < count && font->postScriptName_.empty(); i++) {
      size_t rec = name->offset + 6 + size_t(i) * 12;
      uint16_t platform = r.u16(rec), nameId = r.u16(rec + 6);
      uint16_t length = r.u16(rec + 8), offset = r.u16(rec + 10);
      if (nameId != 6) continue;
      r.check(strings + offset, length);
      std::string value;
      if (platform == 3 || platform == 0) {
        for (size_t k = 0; k + 1 < length; k += 2) value += char(r.u8(strings + offset + k + 1));
      } else {
        for (size_t k = 0; k < length; k++) value += char(r.u8(strings + offset + k));
      }
      font->postScriptName_ = value;
    }
  }
  font->postScriptName_ = sanitizePostScriptName(font->postScriptName_);

  // Prefer full-Unicode (format 12) subtables, then BMP (format 4), then symbol.
  uint16_t numCmaps = r.u16(cmap->offset + 2);
  int bestScore = 0;
  for (uint16_t i = 0; i < numCmaps; i++) {
    size_t rec = cmap->offset + 4 + size_t(i) * 8;
    uint16_t platform = r.u16(rec), encoding = r.u16(rec + 2);
    size_t sub = cmap->offset + r.u32(rec + 4);
    uint16_t format = r.u16(sub);
    int score = 0;
    if (format == 12 && ((platform == 3 && encoding == 10) || platform == 0)) score = 4;
    else if (format == 4 && ((platform == 3 && encoding == 1) || platform == 0)) score = 3;
    else if (format == 4 && platform == 3 && encoding == 0) score = 2;
    if (score > bestScore) {
      bestScore = score;
      font->cmapOffset_ = sub;
      font->cmapFormat_ = format;
      font->symbolCmap_ = score == 2;
    }
  }
  if (bestScore == 0) throw std::runtime_error("Font has no supported Unicode cmap");

  if (!font->isCff_) {
    size_t entries = size_t(font->numGlyphs_) + 1;
    const Range* loca = font->table(kLoca);
    if (loca->length < entries * (font->longLoca_ ? 4 : 2)) throw std::runtime_error("Font loca table is truncated");
  }

  for (uint32_t cp = 0; cp < 256; cp++) font->latin1_[cp] = font->lookupCmap(cp);
  return font;
}

uint16_t TrueTypeFont::glyphId(uint32_t codepoint) const {
  if (codepoint < 256) return latin1_[codepoint];
  return lookupCmap(codepoint);
}

uint16_t TrueTypeFont::lookupCmap(uint32_t cp) const {
  Reader r(data_.data(), data_.size());
  uint32_t glyph = 0;
  if (cmapFormat_ == 4) {
    uint32_t code = cp;
    if (symbolCmap_ && cp < 0x100) code = 0xF000 + cp;
    if (code > 0xFFFF) return 0;
    size_t base = cmapOffset_;
    uint16_t segCount = r.u16(base + 6) / 2;
    size_t ends = base + 14, starts = ends + segCount * 2 + 2;
    size_t deltas = starts + segCount * 2, rangeOffsets = deltas + segCount * 2;
    size_t lo = 0, hi = segCount;
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      if (r.u16(ends + mid * 2) < code) lo = mid + 1;
      else hi = mid;
    }
    if (lo == segCount) return 0;
    uint16_t start = r.u16(starts + lo * 2);
    if (start > code) return 0;
    uint16_t delta = r.u16(deltas + lo * 2);
    uint16_t rangeOffset = r.u16(rangeOffsets + lo * 2);
    if (rangeOffset == 0) {
      glyph = (code + delta) & 0xFFFF;
    } else {
      glyph = r.u16(rangeOffsets + lo * 2 + rangeOffset + (code - start) * 2);
      if (glyph != 0) glyph = (glyph + delta) & 0xFFFF;
    }
  } else if (cmapFormat_ == 12) {
    size_t base = cmapOffset_;
    uint32_t groups = r.u32(base + 12);
    size_t lo = 0, hi = groups;
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      size_t g = base + 16 + mid * 12;
      if (r.u32(g + 4) < cp) lo = mid + 1;
      else hi = mid;
    }
    if (lo == groups) return 0;
    size_t g = base + 16 + lo * 12;
    uint32_t start = r.u32(g);
    if (start > cp) return 0;
    glyph = r.u32(g + 8) + (cp - start);
  }
  return glyph < numGlyphs_ ? uint16_t(glyph) : 0;
}

size_t TrueTypeFont::glyphOffset(uint16_t glyph) const {
  Reader r(data_.data(), data_.size());
  const Range* loca = table(kLoca);
  return longLoca_ ? r.u32(loca->offset + size_t(glyph) * 4) : size_t(r.u16(loca->offset + size_t(glyph) * 2)) * 2;
}

std::vector<uint8_t> TrueTypeFont::copyTable(uint32_t tag) const {
  const Range* t = table(tag);
  if (!t) return {};
  return std::vector<uint8_t>(data_.begin() + t->offset, data_.begin() + t->offset + t->length);
}

std::vector<uint8_t> TrueTypeFont::embeddable(const std::vector<bool>& glyphs) const {
  std::vector<Table> tables;
  auto keep = [&](uint32_t tag) {
    if (table(tag)) tables.push_back({tag, copyTable(tag)});
  };

  if (isCff_) {
    for (uint32_t tag : {kCff, kCmap, kHead, kHhea, kHmtx, kMaxp, kName, kOs2, kPost}) keep(tag);
    return assemble(std::move(tables));
  }

  Reader r(data_.data(), data_.size());
  const Range* glyf = table(kGlyf);

  std::vector<bool> used(numGlyphs_, false);
  std::vector<uint16_t> pending = {0};
  for (uint16_t g = 0; g < numGlyphs_ && g < glyphs.size(); g++) {
    if (glyphs[g]) pending.push_back(g);
  }
  while (!pending.empty()) {
    uint16_t g = pending.back();
    pending.pop_back();
    if (used[g]) continue;
    used[g] = true;
    size_t start = glyphOffset(g), end = glyphOffset(g + 1);
    if (end <= start + 10 || end > glyf->length) continue;
    if (r.s16(glyf->offset + start) >= 0) continue;
    // Composite glyph: queue every referenced component.
    size_t p = glyf->offset + start + 10, limit = glyf->offset + end;
    while (p + 4 <= limit) {
      uint16_t flags = r.u16(p);
      uint16_t component = r.u16(p + 2);
      if (component < numGlyphs_) pending.push_back(component);
      p += 4 + ((flags & 0x0001) ? 4 : 2);
      if (flags & 0x0008) p += 2;
      else if (flags & 0x0040) p += 4;
      else if (flags & 0x0080) p += 8;
      if (!(flags & 0x0020)) break;
    }
  }

  std::vector<uint8_t> newGlyf;
  std::vector<uint8_t> newLoca((size_t(numGlyphs_) + 1) * 4);
  for (uint16_t g = 0; g < numGlyphs_; g++) {
    putU32(newLoca, size_t(g) * 4, uint32_t(newGlyf.size()));
    if (!used[g]) continue;
    size_t start = glyphOffset(g), end = glyphOffset(g + 1);
    if (end <= start || end > glyf->length) continue;
    newGlyf.insert(newGlyf.end(), data_.begin() + glyf->offset + start, data_.begin() + glyf->offset + end);
    while (newGlyf.size() % 4) newGlyf.push_back(0);
  }
  putU32(newLoca, size_t(numGlyphs_) * 4, uint32_t(newGlyf.size()));

  std::vector<uint8_t> head = copyTable(kHead);
  putU16(head, 50, 1);
  tables.push_back({kHead, std::move(head)});
  tables.push_back({kLoca, std::move(newLoca)});
  tables.push_back({kGlyf, std::move(newGlyf)});
  for (uint32_t tag : {kHhea, kHmtx, kMaxp, kOs2, kCvt, kFpgm, kPrep}) keep(tag);
  return assemble(std::move(tables));
}

std::vector<uint8_t> TrueTypeFont::assemble(std::vector<Table> tables) {
  std::sort(tables.begin(), tables.end(), [](const Table& a, const Table& b) { return a.tag < b.tag; });
  bool cff = std::any_of(tables.begin(), tables.end(), [](const Table& t) { return t.tag == kCff; });

  uint16_t numTables = uint16_t(tables.size());
  uint16_t entrySelector = 0;
  while ((2u << entrySelector) <= numTables) entrySelector++;
  uint16_t searchRange = uint16_t((1u << entrySelector) * 16);

  size_t headerSize = 12 + size_t(numTables) * 16;
  size_t total = headerSize;
  for (const Table& t : tables) total += (t.data.size() + 3) & ~size_t(3);

  std::vector<uint8_t> out(total, 0);
  putU32(out, 0, cff ? makeTag('O', 'T', 'T', 'O') : 0x00010000);
  putU16(out, 4, numTables);
  putU16(out, 6, searchRange);
  putU16(out, 8, entrySelector);
  putU16(out, 10, uint16_t(numTables * 16 - searchRange));

  size_t offset = headerSize, headOffset = 0;
  for (size_t i = 0; i < tables.size(); i++) {
    Table& t = tables[i];
    if (t.tag == kHead && t.data.size() >= 12) {
      putU32(t.data, 8, 0);
      headOffset = offset;
    }
    size_t rec = 12 + i * 16;
    putU32(out, rec, t.tag);
    putU32(out, rec + 4, checksum(t.data.data(), t.data.size()));
    putU32(out, rec + 8, uint32_t(offset));
    putU32(out, rec + 12, uint32_t(t.data.size()));
    std::copy(t.data.begin(), t.data.end(), out.begin() + offset);
    offset += (t.data.size() + 3) & ~size_t(3);
  }
  if (headOffset) putU32(out, headOffset + 8, 0xB1B0AFBAu - checksum(out.data(), out.size()));
  return out;
}

} // namespace margelo::nitro::nitropdf

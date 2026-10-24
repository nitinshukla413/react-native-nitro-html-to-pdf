#pragma once

#include <cstdint>
#include <string>

namespace margelo::nitro::nitropdf {

/** Decodes one code point and advances `s`. Invalid input yields U+FFFD. */
inline uint32_t decodeUtf8(const char*& s) {
  auto c = static_cast<unsigned char>(*s++);
  if (c < 0x80) return c;
  int extra;
  uint32_t cp;
  if ((c & 0xE0) == 0xC0) {
    extra = 1;
    cp = c & 0x1F;
  } else if ((c & 0xF0) == 0xE0) {
    extra = 2;
    cp = c & 0x0F;
  } else if ((c & 0xF8) == 0xF0) {
    extra = 3;
    cp = c & 0x07;
  } else {
    return 0xFFFD;
  }
  for (int i = 0; i < extra; i++) {
    auto next = static_cast<unsigned char>(*s);
    if ((next & 0xC0) != 0x80) return 0xFFFD;
    cp = (cp << 6) | (next & 0x3F);
    s++;
  }
  if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return 0xFFFD;
  return cp;
}

inline void appendUtf8(std::string& out, uint32_t cp) {
  if (cp < 0x80) {
    out += char(cp);
  } else if (cp < 0x800) {
    out += char(0xC0 | (cp >> 6));
    out += char(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += char(0xE0 | (cp >> 12));
    out += char(0x80 | ((cp >> 6) & 0x3F));
    out += char(0x80 | (cp & 0x3F));
  } else {
    out += char(0xF0 | (cp >> 18));
    out += char(0x80 | ((cp >> 12) & 0x3F));
    out += char(0x80 | ((cp >> 6) & 0x3F));
    out += char(0x80 | (cp & 0x3F));
  }
}

inline void appendHex4(std::string& out, uint32_t value) {
  static const char digits[] = "0123456789ABCDEF";
  out += digits[(value >> 12) & 0xF];
  out += digits[(value >> 8) & 0xF];
  out += digits[(value >> 4) & 0xF];
  out += digits[value & 0xF];
}

inline void appendUtf16Hex(std::string& out, uint32_t cp) {
  if (cp >= 0x10000) {
    cp -= 0x10000;
    appendHex4(out, 0xD800 + (cp >> 10));
    appendHex4(out, 0xDC00 + (cp & 0x3FF));
  } else {
    appendHex4(out, cp);
  }
}

} // namespace margelo::nitro::nitropdf

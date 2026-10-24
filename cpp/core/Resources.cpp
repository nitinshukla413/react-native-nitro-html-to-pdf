#include "Resources.hpp"

#include <cstdio>
#include <stdexcept>

namespace margelo::nitro::nitropdf {

namespace {

bool startsWith(const std::string& s, const char* prefix) {
  return s.rfind(prefix, 0) == 0;
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::string percentDecode(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '%' && i + 2 < s.size() && hexValue(s[i + 1]) >= 0 && hexValue(s[i + 2]) >= 0) {
      out += char(hexValue(s[i + 1]) * 16 + hexValue(s[i + 2]));
      i += 2;
    } else {
      out += s[i];
    }
  }
  return out;
}

bool decodeBase64(const char* data, size_t length, std::vector<uint8_t>& out) {
  out.clear();
  out.reserve(length * 3 / 4);
  uint32_t buffer = 0;
  int bits = 0;
  for (size_t i = 0; i < length; i++) {
    char c = data[i];
    int v;
    if (c >= 'A' && c <= 'Z') v = c - 'A';
    else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
    else if (c >= '0' && c <= '9') v = c - '0' + 52;
    else if (c == '+' || c == '-') v = 62;
    else if (c == '/' || c == '_') v = 63;
    else if (c == '=') break;
    else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
    else return false;
    buffer = (buffer << 6) | uint32_t(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(uint8_t(buffer >> bits));
    }
  }
  return !out.empty();
}

} // namespace

std::string pathFromFileUrl(const std::string& urlOrPath) {
  if (!startsWith(urlOrPath, "file://")) return urlOrPath;
  std::string rest = urlOrPath.substr(7);
  // file://localhost/path and file:///path both map to /path.
  if (startsWith(rest, "localhost/")) rest = rest.substr(9);
  return percentDecode(rest);
}

std::vector<uint8_t> readFile(const std::string& path) {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (!file) throw std::runtime_error("Cannot open file: " + path);
  std::vector<uint8_t> data;
  uint8_t chunk[16 * 1024];
  size_t n;
  while ((n = std::fread(chunk, 1, sizeof(chunk), file)) > 0) data.insert(data.end(), chunk, chunk + n);
  bool failed = std::ferror(file) != 0;
  std::fclose(file);
  if (failed) throw std::runtime_error("Cannot read file: " + path);
  return data;
}

std::string resolveUrl(const std::string& src, const std::string& baseUrl) {
  if (src.empty() || startsWith(src, "data:") || startsWith(src, "file://") || src[0] == '/' ||
      src.find("://") != std::string::npos || baseUrl.empty()) {
    return src;
  }
  std::string base = baseUrl;
  if (base.back() != '/') base += '/';
  return base + src;
}

bool loadResource(const std::string& url, std::vector<uint8_t>& out) {
  if (startsWith(url, "data:")) {
    size_t comma = url.find(',');
    if (comma == std::string::npos) return false;
    std::string meta = url.substr(5, comma - 5);
    if (meta.size() >= 7 && meta.compare(meta.size() - 7, 7, ";base64") == 0) {
      return decodeBase64(url.data() + comma + 1, url.size() - comma - 1, out);
    }
    std::string decoded = percentDecode(url.substr(comma + 1));
    out.assign(decoded.begin(), decoded.end());
    return !out.empty();
  }
  if (url.find("://") != std::string::npos && !startsWith(url, "file://")) return false;
  try {
    out = readFile(pathFromFileUrl(url));
    return !out.empty();
  } catch (const std::exception&) {
    return false;
  }
}

} // namespace margelo::nitro::nitropdf

#include "HybridPdfDocument.hpp"

#include "core/Resources.hpp"

#include <cstdio>
#include <stdexcept>

namespace margelo::nitro::nitropdf {

HybridPdfDocument::HybridPdfDocument(std::vector<uint8_t> bytes, size_t pageCount)
    : HybridObject(TAG), _bytes(std::make_shared<std::vector<uint8_t>>(std::move(bytes))), _pageCount(pageCount) {}

std::shared_ptr<ArrayBuffer> HybridPdfDocument::toArrayBuffer() {
  // The buffer keeps the native bytes alive instead of copying them.
  auto bytes = _bytes;
  return ArrayBuffer::wrap(bytes->data(), bytes->size(), [bytes]() {});
}

std::string HybridPdfDocument::toBase64() {
  static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const std::vector<uint8_t>& in = *_bytes;
  std::string out((in.size() + 2) / 3 * 4, '=');
  size_t o = 0;
  for (size_t i = 0; i < in.size(); i += 3) {
    uint32_t n = uint32_t(in[i]) << 16;
    if (i + 1 < in.size()) n |= uint32_t(in[i + 1]) << 8;
    if (i + 2 < in.size()) n |= in[i + 2];
    out[o++] = kAlphabet[(n >> 18) & 63];
    out[o++] = kAlphabet[(n >> 12) & 63];
    if (i + 1 < in.size()) out[o] = kAlphabet[(n >> 6) & 63];
    o++;
    if (i + 2 < in.size()) out[o] = kAlphabet[n & 63];
    o++;
  }
  return out;
}

std::shared_ptr<Promise<void>> HybridPdfDocument::saveToFile(const std::string& path) {
  auto bytes = _bytes;
  std::string target = pathFromFileUrl(path);
  return Promise<void>::async([bytes, target]() {
    if (target.empty()) throw std::invalid_argument("saveToFile: path must not be empty");
    FILE* file = std::fopen(target.c_str(), "wb");
    if (!file) throw std::runtime_error("saveToFile: cannot open " + target);
    size_t written = std::fwrite(bytes->data(), 1, bytes->size(), file);
    bool closed = std::fclose(file) == 0;
    if (written != bytes->size() || !closed) throw std::runtime_error("saveToFile: failed to write " + target);
  });
}

} // namespace margelo::nitro::nitropdf

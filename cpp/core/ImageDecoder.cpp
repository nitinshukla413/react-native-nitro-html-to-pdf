#include "ImageDecoder.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_MAX_DIMENSIONS 16384
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#endif
#include "../third_party/stb/stb_image.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <climits>

namespace margelo::nitro::nitropdf {

namespace {

std::shared_ptr<PdfImage> parseJpeg(std::vector<uint8_t>& bytes) {
  const size_t size = bytes.size();
  bool adobe = false;
  size_t p = 2;
  while (p + 4 <= size) {
    if (bytes[p] != 0xFF) return nullptr;
    uint8_t marker = bytes[p + 1];
    if (marker == 0xFF) {
      p++;
      continue;
    }
    if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
      p += 2;
      continue;
    }
    if (marker == 0xD9 || marker == 0xDA) return nullptr;
    size_t length = (size_t(bytes[p + 2]) << 8) | bytes[p + 3];
    if (length < 2 || p + 2 + length > size) return nullptr;
    size_t seg = p + 4;
    if (marker == 0xEE && length >= 7 && bytes[seg] == 'A' && bytes[seg + 1] == 'd' && bytes[seg + 2] == 'o' &&
        bytes[seg + 3] == 'b' && bytes[seg + 4] == 'e') {
      adobe = true;
    }
    bool isSof = marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
    if (isSof) {
      if (length < 8) return nullptr;
      auto image = std::make_shared<PdfImage>();
      if (bytes[seg] != 8) return nullptr;
      image->height = (bytes[seg + 1] << 8) | bytes[seg + 2];
      image->width = (bytes[seg + 3] << 8) | bytes[seg + 4];
      image->components = bytes[seg + 5];
      if (image->width <= 0 || image->height <= 0) return nullptr;
      if (image->components != 1 && image->components != 3 && image->components != 4) return nullptr;
      image->isJpeg = true;
      image->invertedCmyk = adobe && image->components == 4;
      image->data = std::move(bytes);
      return image;
    }
    p += 2 + length;
  }
  return nullptr;
}

std::shared_ptr<PdfImage> parsePng(const std::vector<uint8_t>& bytes) {
  if (bytes.size() > INT_MAX) return nullptr;
  int width = 0, height = 0, channels = 0;
  stbi_uc* pixels = stbi_load_from_memory(bytes.data(), int(bytes.size()), &width, &height, &channels, 0);
  if (!pixels) return nullptr;

  auto image = std::make_shared<PdfImage>();
  image->width = width;
  image->height = height;
  bool hasAlpha = channels == 2 || channels == 4;
  int colorChannels = hasAlpha ? channels - 1 : channels;
  image->components = colorChannels;
  size_t count = size_t(width) * size_t(height);
  image->data.resize(count * size_t(colorChannels));

  bool opaque = true;
  if (hasAlpha) image->alpha.resize(count);
  for (size_t i = 0; i < count; i++) {
    const stbi_uc* px = pixels + i * size_t(channels);
    for (int c = 0; c < colorChannels; c++) image->data[i * size_t(colorChannels) + size_t(c)] = px[c];
    if (hasAlpha) {
      image->alpha[i] = px[colorChannels];
      opaque = opaque && px[colorChannels] == 255;
    }
  }
  stbi_image_free(pixels);
  if (opaque) image->alpha.clear();
  return image;
}

} // namespace

std::shared_ptr<const PdfImage> decodeImage(std::vector<uint8_t> bytes) {
  if (bytes.size() >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) return parseJpeg(bytes);
  if (bytes.size() >= 8 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G') return parsePng(bytes);
  return nullptr;
}

} // namespace margelo::nitro::nitropdf

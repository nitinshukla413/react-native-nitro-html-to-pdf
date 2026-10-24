#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace margelo::nitro::nitropdf {

struct PdfImage {
  int width = 0;
  int height = 0;
  /** 1 = gray, 3 = RGB, 4 = CMYK (JPEG only). */
  int components = 3;
  /** `data` holds the original JPEG stream instead of raw samples. */
  bool isJpeg = false;
  /** Adobe CMYK JPEGs store inverted samples. */
  bool invertedCmyk = false;
  std::vector<uint8_t> data;
  std::vector<uint8_t> alpha;
};

/** JPEG is passed through without re-encoding; PNG is decoded. Returns nullptr for other formats. */
std::shared_ptr<const PdfImage> decodeImage(std::vector<uint8_t> bytes);

} // namespace margelo::nitro::nitropdf

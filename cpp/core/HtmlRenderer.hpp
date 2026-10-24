#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace margelo::nitro::nitropdf {

struct RenderOptions {
  std::string html;
  std::string baseUrl;
  /** Page size and margins in PDF points. */
  double pageWidth = 595.28;
  double pageHeight = 841.89;
  double marginTop = 36;
  double marginRight = 36;
  double marginBottom = 36;
  double marginLeft = 36;
};

struct RenderResult {
  std::vector<uint8_t> pdf;
  size_t pageCount = 0;
};

/** Safe to call from any thread. */
RenderResult renderHtmlToPdf(const RenderOptions& options);

} // namespace margelo::nitro::nitropdf

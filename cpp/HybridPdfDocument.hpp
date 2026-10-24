#pragma once

#include "HybridPdfDocumentSpec.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace margelo::nitro::nitropdf {

class HybridPdfDocument : public HybridPdfDocumentSpec {
public:
  HybridPdfDocument(std::vector<uint8_t> bytes, size_t pageCount);

  double getPageCount() override { return double(_pageCount); }
  double getByteLength() override { return double(_bytes->size()); }
  std::shared_ptr<ArrayBuffer> toArrayBuffer() override;
  std::string toBase64() override;
  std::shared_ptr<Promise<void>> saveToFile(const std::string& path) override;

  size_t getExternalMemorySize() noexcept override { return _bytes->size(); }

private:
  std::shared_ptr<std::vector<uint8_t>> _bytes;
  size_t _pageCount;
};

} // namespace margelo::nitro::nitropdf

#pragma once

#include "HybridNitroPdfSpec.hpp"

namespace margelo::nitro::nitropdf {

class HybridNitroPdf : public HybridNitroPdfSpec {
public:
  HybridNitroPdf() : HybridObject(TAG) {}

  std::shared_ptr<Promise<std::shared_ptr<HybridPdfDocumentSpec>>> generate(const PdfOptions& options) override;
  void registerFont(const FontOptions& font) override;
};

} // namespace margelo::nitro::nitropdf

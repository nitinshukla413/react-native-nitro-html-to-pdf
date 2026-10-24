#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace margelo::nitro::nitropdf {

/** Implemented per platform. Returns one font file for a CSS family (generic ones included), or empty. */
std::vector<uint8_t> loadSystemFont(const std::string& family, bool bold, bool italic);

} // namespace margelo::nitro::nitropdf

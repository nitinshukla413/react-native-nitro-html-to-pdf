#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace margelo::nitro::nitropdf {

std::string pathFromFileUrl(const std::string& urlOrPath);

std::vector<uint8_t> readFile(const std::string& path);

std::string resolveUrl(const std::string& src, const std::string& baseUrl);

/** Loads `data:` URLs and local files only; returns false for anything else. */
bool loadResource(const std::string& url, std::vector<uint8_t>& out);

} // namespace margelo::nitro::nitropdf

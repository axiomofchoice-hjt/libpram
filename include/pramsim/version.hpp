#pragma once

#include <cstdint>
#include <string_view>

namespace pram {
inline constexpr std::string_view version = "0.2.0";

inline constexpr uint32_t version_major = 0;
inline constexpr uint32_t version_minor = 2;
inline constexpr uint32_t version_patch = 0;

inline constexpr uint32_t version_number = (version_major * 10000) + (version_minor * 100) + version_patch;
}  // namespace pram

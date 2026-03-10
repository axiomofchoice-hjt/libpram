#pragma once

#include <cstdint>

namespace pram {
namespace impl {
enum class ReadPolicy : uint8_t {
    Exclusive,   // 互斥读
    Concurrent,  // 并发读
};
}  // namespace impl
}  // namespace pram

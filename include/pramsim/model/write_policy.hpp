#pragma once

#include <cstdint>

namespace pram {
namespace impl {
enum class WritePolicy : uint8_t {
    Exclusive,  // 互斥写
    Common,     // 公共写
    Arbitrary,  // 任意写
    Priority,   // 优先级写
    Add,        // 合并写 加法
    Max,        // 合并写 取最大值
    Min,        // 合并写 取最小值
};
}  // namespace impl
}  // namespace pram

#pragma once

#include <cstdint>

namespace pram {
namespace impl {
enum class ReadPolicy : uint8_t {
    Exclusive,   // 互斥读
    Concurrent,  // 并发读
};

enum class WritePolicy : uint8_t {
    Exclusive,  // 互斥写
    Common,     // 公共写
    Arbitrary,  // 任意写
    Add,        // 合并写 加法
    Max,        // 合并写 取最大值
    Min,        // 合并写 取最小值
};
}  // namespace impl

struct Model {
    impl::ReadPolicy read_policy;
    impl::WritePolicy write_policy;
};

constexpr Model EREW = {.read_policy = impl::ReadPolicy::Exclusive, .write_policy = impl::WritePolicy::Exclusive};
constexpr Model CREW = {.read_policy = impl::ReadPolicy::Concurrent, .write_policy = impl::WritePolicy::Exclusive};
constexpr Model CRCW_Common = {.read_policy = impl::ReadPolicy::Concurrent, .write_policy = impl::WritePolicy::Common};
constexpr Model CRCW_Arbitrary = {
    .read_policy = impl::ReadPolicy::Concurrent, .write_policy = impl::WritePolicy::Arbitrary};
constexpr Model CRCW_Add = {.read_policy = impl::ReadPolicy::Concurrent, .write_policy = impl::WritePolicy::Add};
constexpr Model CRCW_Max = {.read_policy = impl::ReadPolicy::Concurrent, .write_policy = impl::WritePolicy::Max};
constexpr Model CRCW_Min = {.read_policy = impl::ReadPolicy::Concurrent, .write_policy = impl::WritePolicy::Min};
}  // namespace pram

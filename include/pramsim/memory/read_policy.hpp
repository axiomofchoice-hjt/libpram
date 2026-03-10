#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "pramsim/base/assert.hpp"

namespace pram {
namespace impl {
enum class ReadPolicy : uint8_t {
    Exclusive,   // 互斥读
    Concurrent,  // 并发读
};

template <typename T>
struct ReadRequest {
    T* internal_ref;
    size_t pid;
};

template <typename T>
void check_exclusive_read(const std::vector<ReadRequest<T>>& read_requests) {
    for (size_t i = 0; i + 1 < read_requests.size(); i++) {
        if (read_requests[i].internal_ref == read_requests[i + 1].internal_ref) {
            assert_or_throw(false, "Read conflict: exclusive read to the same address");
        }
    }
}
}  // namespace impl
}  // namespace pram

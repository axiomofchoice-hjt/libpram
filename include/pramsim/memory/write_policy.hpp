#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <ranges>
#include <vector>

#include "pramsim/base/assert.hpp"

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

template <typename T>
struct WriteRequest {
    T* internal_ref;
    T value;
    size_t pid;
};

template <typename T>
void check_exclusive_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (size_t i = 0; i + 1 < write_requests.size(); i++) {
        if (write_requests[i].internal_ref == write_requests[i + 1].internal_ref) {
            assert_or_throw(false, "Write conflict: exclusive write to the same address");
        }
    }
}

template <typename T>
void check_common_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (size_t i = 0; i + 1 < write_requests.size(); i++) {
        if (write_requests[i].internal_ref == write_requests[i + 1].internal_ref &&
            write_requests[i].value != write_requests[i + 1].value) {
            assert_or_throw(false, "Write conflict: common write with different values");
        }
    }
}

template <typename T>
void apply_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (const auto& req : write_requests) {
        *req.internal_ref = req.value;
    }
}

inline std::mt19937 gen{std::random_device{}()};

template <typename T>
void apply_arbitrary_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (size_t i = 0; i < write_requests.size();) {
        size_t j = i + 1;
        while (j < write_requests.size() && write_requests[i].internal_ref == write_requests[j].internal_ref) {
            j++;
        }
        std::uniform_int_distribution<size_t> uniform(i, j - 1);
        *write_requests[i].internal_ref = write_requests[uniform(gen)].value;
        i = j;
    }
}

template <typename T>
void apply_priority_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (size_t i = 0; i < write_requests.size();) {
        size_t j = i + 1;
        while (j < write_requests.size() && write_requests[i].internal_ref == write_requests[j].internal_ref) {
            j++;
        }
        *write_requests[i].internal_ref = write_requests[i].value;
        i = j;
    }
}

template <typename T>
void apply_combining_write(const std::vector<WriteRequest<T>>& write_requests, const auto& combine_function) {
    for (size_t i : std::views::iota(0zU, write_requests.size())) {
        if (i == 0 || write_requests[i].internal_ref != write_requests[i - 1].internal_ref) {
            *write_requests[i].internal_ref = write_requests[i].value;
        } else {
            *write_requests[i].internal_ref =
                combine_function(*write_requests[i].internal_ref, write_requests[i].value);
        }
    }
}
}  // namespace impl
}  // namespace pram

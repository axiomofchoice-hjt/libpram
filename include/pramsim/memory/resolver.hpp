#pragma once

#include <cstddef>
#include <random>
#include <ranges>
#include <vector>

#include "pramsim/base/assert.hpp"

namespace pram {
namespace impl {
template <typename T>
struct ReadRequest {
    T* internal_ref;
    size_t pid;
};

template <typename T>
struct WriteRequest {
    T* internal_ref;
    T value;
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

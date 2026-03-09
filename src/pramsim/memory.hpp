#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "base/assert.hpp"
#include "model.hpp"

namespace pram {
namespace impl {
template <typename T>
struct ReadRequest {
    T* internal_ref;
};

template <typename T>
struct WriteRequest {
    T* internal_ref;
    T value;
};
}  // namespace impl

struct Memory {
    virtual void commit() = 0;
    virtual size_t read_count() const = 0;
    virtual size_t write_count() const = 0;
    virtual ~Memory() = default;
};

namespace impl {
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

template <typename T>
void apply_combining_write(const std::vector<WriteRequest<T>>& write_requests, const auto& combine_function) {
    for (size_t i = 0; i < write_requests.size(); i++) {
        if (i == 0 || write_requests[i].internal_ref != write_requests[i - 1].internal_ref) {
            *write_requests[i].internal_ref = write_requests[i].value;
        } else {
            *write_requests[i].internal_ref =
                combine_function(*write_requests[i].internal_ref, write_requests[i].value);
        }
    }
}
}  // namespace impl

template <typename T>
struct SharedArray : Memory {
    std::vector<T> data;
    Model model;

    std::vector<impl::ReadRequest<T>> read_requests;
    std::vector<impl::WriteRequest<T>> write_requests;

    size_t read_counter;
    size_t write_counter;

    SharedArray(size_t length, Model model)
        : data(std::vector<T>(length)), model(model), read_counter(0), write_counter(0) {}

    SharedArray(std::vector<T> data, Model model)
        : data(std::move(data)), model(model), read_counter(0), write_counter(0) {}

    size_t size() const { return data.size(); }

    T operator[](size_t index) {
        read_requests.push_back({&data[index]});
        return data[index];
    }

    void write(size_t index, T value) { write_requests.push_back({&data[index], value}); }

    void commit() override {
        // std::println("commit");
        std::ranges::sort(read_requests, [](const impl::ReadRequest<T>& a, const impl::ReadRequest<T>& b) {
            return a.internal_ref < b.internal_ref;
        });
        std::ranges::sort(write_requests, [](const impl::WriteRequest<T>& a, const impl::WriteRequest<T>& b) {
            return a.internal_ref < b.internal_ref;
        });

        {
            for (size_t i = 0, j = 0; i < read_requests.size() && j < write_requests.size();) {
                if (read_requests[i].internal_ref < write_requests[j].internal_ref) {
                    i++;
                } else if (read_requests[i].internal_ref > write_requests[j].internal_ref) {
                    j++;
                } else {
                    assert_or_throw(false, "Read-write conflict: read and write to the same address");
                }
            }
        }

        switch (model.read_policy) {
            case impl::ReadPolicy::Exclusive:  // 处理互斥读
                impl::check_exclusive_read(read_requests);
                break;
            case impl::ReadPolicy::Concurrent:  // 处理并发读
                break;
        }

        switch (model.write_policy) {
            case impl::WritePolicy::Exclusive:  // 处理互斥写
                impl::check_exclusive_write(write_requests);
                impl::apply_write(write_requests);
                break;
            case impl::WritePolicy::Common:  // 处理公共写
                impl::check_common_write(write_requests);
                impl::apply_write(write_requests);
                break;
            case impl::WritePolicy::Arbitrary:  // 处理任意写
                impl::apply_write(write_requests);
                break;
            case impl::WritePolicy::Add:  // 处理合并写 加法
                impl::apply_combining_write(write_requests, std::plus<T>{});
                break;
            case impl::WritePolicy::Max:  // 处理合并写 取最大值
                impl::apply_combining_write(write_requests, [](const T& a, const T& b) { return std::max(a, b); });
                break;
            case impl::WritePolicy::Min:  // 处理合并写 取最小值
                impl::apply_combining_write(write_requests, [](const T& a, const T& b) { return std::min(a, b); });
                break;
        }

        read_counter += read_requests.size();
        write_counter += write_requests.size();

        read_requests.clear();
        write_requests.clear();
    }

    size_t read_count() const override { return read_counter; }
    size_t write_count() const override { return write_counter; }
};
}  // namespace pram

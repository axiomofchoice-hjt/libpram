#pragma once

#include <algorithm>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <print>
#include <utility>
#include <vector>

#include "base/assert.h"
#include "model.h"

namespace pram {
template <typename T>
struct ReadAwaitable {
    T value;
    bool await_ready() const noexcept { return false; }
    bool await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept { return true; }
    T await_resume() noexcept { return value; }
};

template <typename T>
struct WriteAwaitable {
    bool await_ready() const noexcept { return false; }
    bool await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept { return true; }
    void await_resume() noexcept {}
};

struct Memory {
    virtual void start_round() = 0;
    virtual void end_round() = 0;

    virtual ~Memory() = default;
};

namespace impl {
template <typename T>
struct WriteRequest {
    T* address;
    T value;
};

namespace {
template <typename T>
void check_exclusive_read(const std::vector<T*>& read_requests) {
    for (size_t i = 0; i + 1 < read_requests.size(); i++) {
        if (read_requests[i] == read_requests[i + 1]) {
            assert_or_throw(false, "Read conflict: exclusive read to the same address");
        }
    }
}

template <typename T>
void check_exclusive_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (size_t i = 0; i + 1 < write_requests.size(); i++) {
        if (write_requests[i].address == write_requests[i + 1].address) {
            assert_or_throw(false, "Write conflict: exclusive write to the same address");
        }
    }
}

template <typename T>
void check_common_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (size_t i = 0; i + 1 < write_requests.size(); i++) {
        if (write_requests[i].address == write_requests[i + 1].address &&
            write_requests[i].value != write_requests[i + 1].value) {
            assert_or_throw(false, "Write conflict: common write with different values");
        }
    }
}

template <typename T>
void apply_exclusive_common_write(const std::vector<WriteRequest<T>>& write_requests) {
    for (const auto& req : write_requests) {
        *req.address = req.value;
    }
}

template <typename T>
void apply_combining_write(const std::vector<WriteRequest<T>>& write_requests, const auto& combine_function) {
    for (size_t i = 0; i < write_requests.size(); i++) {
        if (i == 0 || write_requests[i].address != write_requests[i - 1].address) {
            *write_requests[i].address = write_requests[i].value;
        } else {
            *write_requests[i].address = combine_function(*write_requests[i].address, write_requests[i].value);
        }
    }
}
}  // namespace
}  // namespace impl

template <typename T>
struct SharedArray : Memory {
    std::vector<T> data;
    Model model;

    std::vector<T*> read_requests;
    std::vector<impl::WriteRequest<T>> write_requests;

    SharedArray(size_t length, Model model) : data(std::vector<T>(length)), model(model) {}

    SharedArray(std::vector<T> data, Model model) : data(std::move(data)), model(model) {}

    ReadAwaitable<T> read(size_t index) {
        read_requests.push_back(&data[index]);
        return ReadAwaitable<T>{data[index]};
    }

    WriteAwaitable<T> write(size_t index, T value) {
        write_requests.push_back({&data[index], value});
        return WriteAwaitable<T>{};
    }

    void start_round() override { std::println("start_round"); }

    void end_round() override {
        std::println("end_round");
        std::ranges::sort(read_requests);
        std::ranges::sort(write_requests,
            [](const impl::WriteRequest<T>& a, const impl::WriteRequest<T>& b) { return a.address < b.address; });

        if (model.read_policy == impl::ReadPolicy::Exclusive) {  // 处理互斥读
            impl::check_exclusive_read(read_requests);
        }
        if (model.write_policy == impl::WritePolicy::Exclusive) {  // 处理互斥写
            impl::check_exclusive_write(write_requests);
            impl::apply_exclusive_common_write(write_requests);
        }
        if (model.write_policy == impl::WritePolicy::Common) {  // 处理公共写
            impl::check_common_write(write_requests);
            impl::apply_exclusive_common_write(write_requests);
        }
        if (model.write_policy == impl::WritePolicy::Add) {  // 处理合并写
            impl::apply_combining_write(write_requests, std::plus<T>{});
        } else if (model.write_policy == impl::WritePolicy::Max) {
            impl::apply_combining_write(write_requests, [](const T& a, const T& b) { return std::max(a, b); });
        } else if (model.write_policy == impl::WritePolicy::Min) {
            impl::apply_combining_write(write_requests, [](const T& a, const T& b) { return std::min(a, b); });
        }

        read_requests.clear();
        write_requests.clear();
    }
};
}  // namespace pram

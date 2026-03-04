#pragma once

#include <algorithm>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <print>
#include <utility>
#include <vector>

#include "runtime_fwd.h"

namespace pram {
template <typename T>
struct LoadAwaitable {
    T value;
    bool await_ready() const noexcept { return false; }
    bool await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept { return true; }
    T await_resume() noexcept { return value; }
};

template <typename T>
struct StoreAwaitable {
    bool await_ready() const noexcept { return false; }
    bool await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept { return true; }
    void await_resume() noexcept {}
};

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

struct MemoryConfig {
    ReadPolicy read_policy;
    WritePolicy write_policy;
};

struct Memory {
    virtual void start_round() = 0;
    virtual void end_round() = 0;

    virtual ~Memory() = default;
};

template <typename T>
struct Array : Memory {
    std::vector<T> data;
    Runtime* runtime;
    MemoryConfig config;

    std::vector<T*> read_requests;
    struct WriteRequest {
        T* address;
        T value;
    };
    std::vector<WriteRequest> write_requests;

    Array(size_t length, Runtime* runtime, MemoryConfig config)
        : data(std::vector<T>(length)), runtime(runtime), config(config) {}

    Array(std::vector<T> data, Runtime* runtime, MemoryConfig config)
        : data(std::move(data)), runtime(runtime), config(config) {}

    LoadAwaitable<T> load(size_t index) {
        read_requests.push_back(&data[index]);
        return LoadAwaitable<T>{data[index]};
    }

    StoreAwaitable<T> store(size_t index, T value) {
        write_requests.push_back({&data[index], value});
        return StoreAwaitable<T>{};
    }

    void start_round() override { std::println("start_round"); }
    void end_round() override {
        std::println("end_round");
        if (config.read_policy == ReadPolicy::Exclusive) {  // 处理互斥读
            std::ranges::sort(read_requests);
            for (size_t i = 0; i + 1 < read_requests.size(); i++) {
                if (read_requests[i] == read_requests[i + 1]) {
                    std::println("Read conflict");
                }
            }
        }
        if (config.write_policy == WritePolicy::Exclusive) {  // 处理互斥写
            std::ranges::sort(
                write_requests, [](const WriteRequest& a, const WriteRequest& b) { return a.address < b.address; });
            for (size_t i = 0; i + 1 < write_requests.size(); i++) {
                if (write_requests[i].address == write_requests[i + 1].address) {
                    std::println("Write conflict");
                }
            }
            for (const auto& req : write_requests) {
                *req.address = req.value;
            }
        }
        if (config.write_policy == WritePolicy::Common) {  // 处理公共写
            std::ranges::sort(
                write_requests, [](const WriteRequest& a, const WriteRequest& b) { return a.address < b.address; });
            for (size_t i = 0; i + 1 < write_requests.size(); i++) {
                if (write_requests[i].address == write_requests[i + 1].address &&
                    write_requests[i].value != write_requests[i + 1].value) {
                    std::println("Write conflict");
                }
            }
            for (const auto& req : write_requests) {
                *req.address = req.value;
            }
        }
        auto combining = [](std::vector<WriteRequest>& write_requests, auto combine_function) {
            std::ranges::sort(
                write_requests, [](const WriteRequest& a, const WriteRequest& b) { return a.address < b.address; });
            for (size_t i = 0; i < write_requests.size(); i++) {
                if (i == 0 || write_requests[i].address != write_requests[i - 1].address) {
                    *write_requests[i].address = write_requests[i].value;
                } else {
                    *write_requests[i].address = combine_function(*write_requests[i].address, write_requests[i].value);
                }
            }
        };
        if (config.write_policy == WritePolicy::Add) {  // 处理合并写
            combining(write_requests, std::plus<T>{});
        } else if (config.write_policy == WritePolicy::Max) {
            combining(write_requests, [](const T& a, const T& b) { return std::max(a, b); });
        } else if (config.write_policy == WritePolicy::Min) {
            combining(write_requests, [](const T& a, const T& b) { return std::min(a, b); });
        }

        read_requests.clear();
        write_requests.clear();
    }
};
}  // namespace pram

#pragma once

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
    void await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept {}
    T await_resume() noexcept { return value; }
};

template <typename T>
struct StoreAwaitable {
    T* address;
    T value;
    bool await_ready() const noexcept { return false; }
    void await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept {}
    T& await_resume() noexcept {
        *address = value;
        return *address;
    }
};

enum class ReadPolicy : uint8_t {
    Exclusive,   // 互斥读
    Concurrent,  // 并发读
};

enum class WritePolicy : uint8_t {
    Exclusive,  // 互斥写
    Common,     // 公共写
    Arbitrary,  // 任意写
    Priority,   // 优先写
    Combining,  // 合并写
};

struct MemoryConfig {
    ReadPolicy read_policy;
    WritePolicy write_policy;
    std::function<void()> combine_function;  // 仅当 WritePolicy::Combining 时有效
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

    Array(size_t length, Runtime* runtime, MemoryConfig config)
        : data(std::vector<T>(length)), runtime(runtime), config(std::move(config)) {}

    Array(std::vector<T> data, Runtime* runtime, MemoryConfig config)
        : data(std::move(data)), runtime(runtime), config(std::move(config)) {}

    LoadAwaitable<T> load(size_t index) { return LoadAwaitable<T>{data[index]}; }

    StoreAwaitable<T> store(size_t index, T value) { return StoreAwaitable<T>{&data[index], value}; }

    void start_round() override { std::println("start_round"); }
    void end_round() override { std::println("end_round"); }
};
}  // namespace pram

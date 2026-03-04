#pragma once

#include <coroutine>
#include <cstddef>
#include <print>
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

struct Memory {
    virtual void start_round() = 0;
    virtual void end_round() = 0;

    virtual ~Memory() = default;
};

template <typename T>
struct Array : Memory {
    std::vector<T> data;
    Runtime* runtime;

    Array(size_t length, Runtime* runtime) : data(std::vector<T>(length)), runtime(runtime) {}

    Array(std::vector<T> data, Runtime* runtime) : data(std::move(data)), runtime(runtime) {}

    LoadAwaitable<T> load(size_t index) { return LoadAwaitable<T>{data[index]}; }

    StoreAwaitable<T> store(size_t index, T value) { return StoreAwaitable<T>{&data[index], value}; }

    void start_round() override { std::println("start_round"); }
    void end_round() override { std::println("end_round"); }
};
}  // namespace pram

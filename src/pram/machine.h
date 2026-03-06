#pragma once

#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <vector>

#include "memory.h"

namespace pram {
struct Task {
    struct promise_type {
        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend([[maybe_unused]] this auto&& self) { return {}; }
        std::suspend_always final_suspend([[maybe_unused]] this auto&& self) noexcept { return {}; }
        void return_void() {}
        void unhandled_exception([[maybe_unused]] this auto&& self) { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    void destroy(this auto&& self) {
        if (self.handle) {
            self.handle.destroy();
            self.handle = nullptr;
        }
    }

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

    Task(const Task&) = delete;
    Task& operator=(this auto&& self, const Task&) = delete;
    Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    Task& operator=(this auto&& self, Task&& other) noexcept {
        if (&self != &other) {
            self.destroy();
            self.handle = other.handle;
            other.handle = nullptr;
        }
        return self;
    }

    ~Task() { destroy(); }
};

struct BarrierAwaitable {
    bool await_ready([[maybe_unused]] this auto&& self) noexcept { return false; }
    void await_suspend([[maybe_unused]] std::coroutine_handle<> _) noexcept {}
    void await_resume() noexcept {}
};

struct Machine {
    std::vector<std::unique_ptr<Memory>> memories;
    Model model;

    Machine() : model{CREW} {}
    Machine(Model model) : model(model) {}

    template <typename T>
    SharedArray<T>& allocate(this auto&& self, size_t length) {
        auto array = std::make_unique<SharedArray<T>>(length, self.model);
        self.memories.push_back(std::move(array));
        return *static_cast<SharedArray<T>*>(self.memories.back().get());
    }

    template <typename T>
    SharedArray<T>& allocate(this auto&& self, std::vector<T> data) {
        auto array = std::make_unique<SharedArray<T>>(std::move(data), self.model);
        self.memories.push_back(std::move(array));
        return *static_cast<SharedArray<T>*>(self.memories.back().get());
    }

    void parallel(size_t n_processors, auto&& func) {
        bool active = true;
        std::vector<Task> tasks;

        for (size_t pid = 0; pid < n_processors; ++pid) {
            tasks.push_back(func(pid));
        }

        while (active) {
            active = false;
            for (auto& t : tasks) {
                if (!t.handle.done()) {
                    active = true;
                    t.handle.resume();
                }
            }
            for (auto& mem : memories) {
                mem->commit();
            }
        }
    }
};

BarrierAwaitable barrier() { return BarrierAwaitable{}; }
}  // namespace pram

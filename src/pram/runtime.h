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
};

struct Runtime {
    std::vector<Memory*> memories;

    template <typename T>
    std::unique_ptr<Array<T>> allocate(this auto&& self, size_t length) {
        auto array = std::make_unique<Array<T>>(length, &self);
        self.memories.push_back(array.get());
        return array;
    }

    template <typename T>
    std::unique_ptr<Array<T>> allocate(this auto&& self, std::vector<T> data) {
        auto array = std::make_unique<Array<T>>(std::move(data), &self);
        self.memories.push_back(array.get());
        return array;
    }

    void parallel(size_t n_processors, auto&& func) {
        bool active = true;
        std::vector<Task> tasks;

        for (size_t pid = 0; pid < n_processors; ++pid) {
            tasks.push_back(func(pid));
        }

        while (active) {
            active = false;

            for (auto& mem : memories) {
                mem->start_round();
            }
            for (auto& t : tasks) {
                if (!t.handle.done()) {
                    active = true;
                    t.handle.resume();
                }
            }
            for (auto& mem : memories) {
                mem->end_round();
            }
        }
    }
};
}  // namespace pram

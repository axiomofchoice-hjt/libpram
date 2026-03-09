#pragma once

#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <ranges>
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

struct Stat {
    size_t n_processors;
    size_t n_rounds;
    size_t n_reads;
    size_t n_writes;
};

struct Machine {
    size_t n_processors;
    Model model;
    std::vector<std::unique_ptr<Memory>> memories;

    size_t round_counter;

    Machine(size_t n_processors) : n_processors(n_processors), model{CREW}, round_counter(0) {}
    Machine(size_t n_processors, Model model) : n_processors(n_processors), model(model), round_counter(0) {}

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

    void parallel(this auto&& self, auto&& func) {
        bool active = true;
        auto tasks =
            std::views::iota(0zU, self.n_processors) | std::views::transform(func) | std::ranges::to<std::vector>();

        while (active) {
            active = false;
            for (auto& t : tasks) {
                if (!t.handle.done()) {
                    active = true;
                    t.handle.resume();
                }
            }
            if (active) {
                for (auto& mem : self.memories) {
                    mem->commit();
                }
                self.round_counter++;
            }
        }
    }

    Stat stat() const {
        size_t n_reads = std::ranges::fold_left(
            memories, 0ULL, [](size_t acc, const std::unique_ptr<Memory>& mem) { return acc + mem->read_count(); });
        size_t n_writes = std::ranges::fold_left(
            memories, 0ULL, [](size_t acc, const std::unique_ptr<Memory>& mem) { return acc + mem->write_count(); });
        return {.n_processors = n_processors, .n_rounds = round_counter, .n_reads = n_reads, .n_writes = n_writes};
    }
};

BarrierAwaitable barrier() { return BarrierAwaitable{}; }
}  // namespace pram

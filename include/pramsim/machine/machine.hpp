#pragma once

#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <ranges>
#include <vector>

#include "pramsim/memory/shared_array.hpp"

namespace pram {
struct Task {
    struct promise_type {
        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    void destroy() {
        if (handle) {
            handle.destroy();
            handle = nullptr;
        }
    }

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    ~Task() { destroy(); }
};

struct StepAwaitable {
    bool await_ready() noexcept { return false; }
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
    Model _model;
    std::vector<std::unique_ptr<Memory>> _memories;
    std::unique_ptr<Context> _context;

    size_t _n_rounds = 0;

    Machine(size_t n_processors) : _model{CREW}, _context{std::make_unique<Context>(n_processors)} {}
    Machine(size_t n_processors, Model model) : _model(model), _context{std::make_unique<Context>(n_processors)} {}

    template <typename T>
    SharedArray<T>& allocate(this auto&& self, size_t length, T value) {
        auto array = std::make_unique<SharedArray<T>>(length, value, self._model, self._context.get());
        self._memories.push_back(std::move(array));
        return *static_cast<SharedArray<T>*>(self._memories.back().get());
    }

    template <typename T>
    SharedArray<T>& allocate(this auto&& self, std::vector<T> data) {
        auto array = std::make_unique<SharedArray<T>>(std::move(data), self._model, self._context.get());
        self._memories.push_back(std::move(array));
        return *static_cast<SharedArray<T>*>(self._memories.back().get());
    }

    template <std::invocable<size_t> F>
    void parallel(this auto&& self, F&& func) {
        bool active = true;
        auto tasks = std::views::iota(0zU, self._context->n_processors) | std::views::transform(func) |
                     std::ranges::to<std::vector>();

        while (active) {
            active = false;
            for (auto&& [pid, t] : tasks | std::views::enumerate) {
                if (!t.handle.done()) {
                    active = true;
                    self._context->current_pid = pid;
                    t.handle.resume();
                }
            }
            self._context->current_pid = std::nullopt;
            if (active) {
                for (auto& mem : self._memories) {
                    mem->commit_round();
                }
                self._n_rounds++;
            }
        }
    }

    Stat stat() const {
        size_t n_reads = std::ranges::fold_left(
            _memories, 0ULL, [](size_t acc, const std::unique_ptr<Memory>& mem) { return acc + mem->n_reads(); });
        size_t n_writes = std::ranges::fold_left(
            _memories, 0ULL, [](size_t acc, const std::unique_ptr<Memory>& mem) { return acc + mem->n_writes(); });
        return {
            .n_processors = _context->n_processors, .n_rounds = _n_rounds, .n_reads = n_reads, .n_writes = n_writes};
    }
};

constexpr StepAwaitable step() { return {}; }
}  // namespace pram

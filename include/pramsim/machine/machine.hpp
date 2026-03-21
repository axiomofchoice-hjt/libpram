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

class Machine {
   public:
    Machine(size_t n_processors) : model_{CREW}, context_{std::make_unique<Context>(n_processors)} {}
    Machine(size_t n_processors, Model model) : model_(model), context_{std::make_unique<Context>(n_processors)} {}

    template <typename T>
    SharedArray<T>& allocate(size_t length, T value) {
        auto array = std::make_unique<SharedArray<T>>(length, value, model_, context_.get());
        SharedArray<T>* res = array.get();
        memories_.push_back(std::move(array));
        return *res;
    }

    template <typename T>
    SharedArray<T>& allocate(std::vector<T> data) {
        auto array = std::make_unique<SharedArray<T>>(std::move(data), model_, context_.get());
        SharedArray<T>* res = array.get();
        memories_.push_back(std::move(array));
        return *res;
    }

    template <std::invocable<size_t> F>
    void parallel(F&& func) {
        bool active = true;
        auto tasks = std::views::iota(0zU, context_->n_processors) | std::views::transform(func) |
                     std::ranges::to<std::vector>();

        while (active) {
            active = false;
            for (auto&& [pid, t] : tasks | std::views::enumerate) {
                if (!t.handle.done()) {
                    active = true;
                    context_->current_pid = pid;
                    t.handle.resume();
                }
            }
            context_->current_pid = std::nullopt;
            if (active) {
                for (auto& mem : memories_) {
                    mem->commit_round();
                }
                n_rounds_++;
            }
        }
    }

    Stat stat() const {
        size_t n_reads = std::ranges::fold_left(
            memories_, 0ULL, [](size_t acc, const std::unique_ptr<Memory>& mem) { return acc + mem->n_reads(); });
        size_t n_writes = std::ranges::fold_left(
            memories_, 0ULL, [](size_t acc, const std::unique_ptr<Memory>& mem) { return acc + mem->n_writes(); });
        return {
            .n_processors = context_->n_processors, .n_rounds = n_rounds_, .n_reads = n_reads, .n_writes = n_writes};
    }

   private:
    Model model_;
    std::vector<std::unique_ptr<Memory>> memories_;
    std::unique_ptr<Context> context_;
    size_t n_rounds_ = 0;
};

constexpr StepAwaitable step() { return {}; }
}  // namespace pram

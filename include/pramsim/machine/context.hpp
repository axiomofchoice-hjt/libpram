#pragma once

#include <cstddef>
#include <optional>
#include <random>

namespace pram {
struct Context {
    size_t n_processors;
    std::optional<size_t> current_pid;
    std::mt19937 rng{std::random_device{}()};

    Context() = default;
    Context(size_t n_processors) : n_processors{n_processors} {}
};
}  // namespace pram

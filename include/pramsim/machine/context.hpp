#pragma once

#include <cstddef>
#include <optional>

namespace pram {
struct Context {
    size_t n_processors;
    std::optional<size_t> current_pid;

    Context() = default;
    Context(size_t n_processors) : n_processors{n_processors} {}
};
}  // namespace pram

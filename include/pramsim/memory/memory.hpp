#pragma once

#include <cstddef>

namespace pram {
struct Memory {
    virtual void commit() = 0;
    virtual size_t n_reads() const = 0;
    virtual size_t n_writes() const = 0;
    virtual ~Memory() = default;
};
}  // namespace pram

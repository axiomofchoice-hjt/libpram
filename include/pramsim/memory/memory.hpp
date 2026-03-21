#pragma once

#include <cstddef>

namespace pram {
class Memory {
   public:
    virtual void commit_round() = 0;
    virtual size_t n_reads() const = 0;
    virtual size_t n_writes() const = 0;
    virtual ~Memory() = default;
};
}  // namespace pram

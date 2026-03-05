#pragma once

#include <format>
#include <source_location>
#include <stdexcept>

namespace pram {
struct assertion_error : std::logic_error {
    using std::logic_error::logic_error;
};

inline void assert_or_throw(bool condition, std::string_view message = "empty message",
    const std::source_location& loc = std::source_location::current()) {
    if (!condition) [[unlikely]] {
        throw assertion_error(std::format("Assertion failed at {}:{}: {}", loc.file_name(), loc.line(), message));
    }
}
}  // namespace pram

#include <print>
#include <ranges>

#include "base/assert.h"
#include "pram/runtime.h"

int main() try {
    pram::Runtime runtime{};
    runtime.default_memory_config = {
        .read_policy = pram::ReadPolicy::Concurrent, .write_policy = pram::WritePolicy::Exclusive};

    constexpr size_t size = 16;
    auto array = runtime.allocate<int>(std::views::iota(0zU, size) | std::ranges::to<std::vector<int>>());

    runtime.parallel(size, [&](size_t pid) -> pram::Task {
        std::println("pid={}", pid);
        std::println("load={}", co_await array->load(pid));
        co_await array->store(pid, 1);
        std::println("load={}", co_await array->load(pid));
    });
} catch (const assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
} catch (const std::exception& e) {
    std::println("Exception: {}", e.what());
    return 1;
}

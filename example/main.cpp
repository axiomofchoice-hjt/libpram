#include <print>
#include <ranges>

#include "pram/pram.h"

int main() try {
    pram::Machine machine{pram::CREW};

    constexpr size_t size = 16;
    auto array = machine.allocate<int>(std::views::iota(0zU, size) | std::ranges::to<std::vector<int>>());

    machine.parallel(size, [&](size_t pid) -> pram::Task {
        std::println("pid={}", pid);
        std::println("read {}", co_await array->read(pid));
        co_await array->write(pid, 1);
        std::println("read {}", co_await array->read(pid));
    });
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
} catch (const std::exception& e) {
    std::println("Exception: {}", e.what());
    return 1;
}

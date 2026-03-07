#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "format.h"  // IWYU pragma: keep

void rank_sort() {
    constexpr size_t size = 8;

    pram::Machine machine{size * size, pram::CRCW_Add};

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(size + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto& input = machine.allocate<int>(data);
    auto& output = machine.allocate<int>(size);
    auto& rank = machine.allocate<size_t>(size);

    std::println("input: {}", input);

    machine.parallel([&](size_t pid) -> pram::Task {
        size_t i = pid / size;
        size_t j = pid % size;
        int value_i = input[i];
        int value_j = input[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);
        }
        co_await pram::barrier();
        if (j == 0) {
            output.write(rank[i], value_i);
        }
    });

    std::println("output: {}", output);
    std::ranges::sort(data);
    pram::assert_or_throw(output.data == data, "Sorted output does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", machine.n_processors, machine.round_count(),
        machine.read_count(), machine.write_count());
}

int main() try {
    std::println("===== example: rank_sort =====");
    rank_sort();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

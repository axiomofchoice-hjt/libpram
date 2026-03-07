#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "format.h"  // IWYU pragma: keep

struct RankSortImpl {
    pram::SharedArray<int>& input;
    pram::SharedArray<size_t>& rank;
    pram::SharedArray<int>& output;

    pram::Task operator()(size_t pid) {
        size_t n = input.size();
        size_t i = pid / n;
        size_t j = pid % n;

        int value_i = input[i];
        int value_j = input[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);
        }
        co_await pram::barrier();
        if (j == 0) {
            output.write(rank[i], value_i);
        }
    }
};

void rank_sort() {
    constexpr size_t n = 8;

    pram::Machine machine{n * n, pram::CRCW_Add};

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(n + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto& input = machine.allocate<int>(data);
    auto& output = machine.allocate<int>(n);
    auto& rank = machine.allocate<size_t>(n);

    std::println("input: {}", input);

    machine.parallel(RankSortImpl{.input = input, .rank = rank, .output = output});

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

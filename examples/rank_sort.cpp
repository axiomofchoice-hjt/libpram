#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "str.h"

/**
 * Rank Sort，CRCW_Add 模型，处理器数 O(n^2)，时间复杂度 O(1)
 */
struct RankSortImpl {
    pram::SharedArray<int>& array;
    pram::SharedArray<size_t>& rank;

    pram::Task operator()(size_t pid) {
        size_t n = array.size();
        size_t i = pid / n;
        size_t j = pid % n;

        int value_i = array[i];
        int value_j = array[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);
        }
        co_await pram::barrier();
        if (j == 0) {
            array.write(rank[i], value_i);
        }
    }
};

void rank_sort() {
    constexpr size_t n = 8;

    pram::Machine machine{n * n, pram::CRCW_Add};

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(n + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto expected = data;
    std::ranges::sort(expected);

    auto& array = machine.allocate<int>(data);
    auto& rank = machine.allocate<size_t>(n);

    std::println("input: {}", str(array));

    machine.parallel(RankSortImpl{.array = array, .rank = rank});

    std::println("output: {}", str(array));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(array.data == expected, "The result does not match expected values.");
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

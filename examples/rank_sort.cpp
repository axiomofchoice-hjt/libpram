#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "str.h"

/**
 * Rank Sort，CRCW_Add 模型，处理器数 O(n^2)，时间复杂度 O(1)
 */
std::pair<std::vector<int>, pram::Stat> rank_sort_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n * n, pram::CRCW_Add};

    auto& array = machine.allocate<int>(data);
    auto& rank = machine.allocate<size_t>(n);

    machine.parallel([&](size_t pid) -> pram::Task {
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
    });

    return {array.data, machine.stat()};
}

void rank_sort() {
    constexpr size_t n = 8;

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(n + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto expected = data;
    std::ranges::sort(expected);

    std::println("input: {}", str(data));

    auto [result, stat] = rank_sort_impl(data);

    std::println("result: {}", str(result));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(result == expected, "The result does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", stat.n_processors, stat.n_rounds, stat.n_reads,
        stat.n_writes);
}

int main() try {
    std::println("===== example: rank_sort =====");
    rank_sort();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

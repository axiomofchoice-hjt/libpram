#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "str.h"

/**
 * 树形求和，CREW 模型，处理器数 O(n)，时间复杂度 O(logn)
 */
std::pair<int, pram::Stat> tree_sum_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n, pram::CREW};

    auto& array = machine.allocate<int>(data);
    auto& result = machine.allocate<int>(1);

    machine.parallel([&](size_t pid) -> pram::Task {
        for (size_t stride = 1; stride < n; stride *= 2) {
            int value = 0;
            if (pid % (2 * stride) == 0 && pid + stride < n) {
                value = array[pid] + array[pid + stride];
            }
            co_await pram::barrier();
            if (pid % (2 * stride) == 0 && pid + stride < n) {
                array.write(pid, value);
            }
            co_await pram::barrier();
        }

        if (pid == 0) {
            result.write(0, array[0]);
        }
    });

    return {result.data[0], machine.stat()};
}

void tree_sum_example() {
    constexpr size_t n = 8;

    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis(1, 4);
    std::vector<int> data;
    std::ranges::for_each(std::views::iota(0zU, n), [&](int) { data.push_back(dis(gen)); });
    std::ranges::shuffle(data, gen);
    int expected = std::ranges::fold_left(data, 0, std::plus{});

    std::println("input: {}", str(data));

    auto [result, stat] = tree_sum_impl(data);

    std::println("result: {}", result);
    std::println("expected: {}", expected);
    pram::assert_or_throw(result == expected, "The result does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", stat.n_processors, stat.n_rounds, stat.n_reads,
        stat.n_writes);
}

int main() try {
    std::println("===== example: tree_sum =====");
    tree_sum_example();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

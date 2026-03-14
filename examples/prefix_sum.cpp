#include <pramsim/pramsim.hpp>
#include <print>
#include <random>
#include <ranges>

#include "str.hpp"

/**
 * 前缀和，CREW 模型，处理器数 O(n)，时间复杂度 O(logn)
 */
std::pair<std::vector<int>, pram::Stat> prefix_sum_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n, pram::CREW};
    auto& array = machine.allocate<int>(data);
    machine.parallel([&](size_t pid) -> pram::Task {
        for (size_t stride = 1; stride < n; stride *= 2) {
            int temp = 0;
            if (pid >= stride) {
                temp = array[pid] + array[pid - stride];
            }
            co_await pram::step();
            if (pid >= stride) {
                array.write(pid, temp);
            }
            co_await pram::step();
        }
    });
    return {array.debug_data(), machine.stat()};
}

void prefix_sum_example() {
    constexpr size_t n = 8;

    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis(1, 4);
    std::vector<int> data;
    std::ranges::for_each(std::views::iota(0zU, n), [&](int) { data.push_back(dis(gen)); });
    std::ranges::shuffle(data, gen);
    std::vector<int> expected;
    std::partial_sum(data.begin(), data.end(), std::back_inserter(expected));

    std::println("input: {}", str(data));

    auto [result, stat] = prefix_sum_impl(data);

    std::println("result: {}", str(result));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(result == expected, "The result does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", stat.n_processors, stat.n_rounds, stat.n_reads,
        stat.n_writes);
}

int main() try {
    std::println("===== example: prefix_sum =====");
    prefix_sum_example();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

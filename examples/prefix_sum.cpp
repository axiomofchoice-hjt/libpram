#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "str.h"

/**
 * 前缀和，CREW 模型，处理器数 O(n)，时间复杂度 O(logn)
 */
struct PrefixSumImpl {
    pram::SharedArray<int>& array;

    pram::Task operator()(size_t pid) {
        size_t n = array.size();

        for (size_t stride = 1; stride < n; stride *= 2) {
            int temp = 0;
            if (pid >= stride) {
                temp = array[pid] + array[pid - stride];
            }
            co_await pram::barrier();
            if (pid >= stride) {
                array.write(pid, temp);
            }
            co_await pram::barrier();
        }
    }
};

void list_ranking() {
    constexpr size_t n = 8;

    pram::Machine machine{n, pram::CREW};

    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis(1, 4);
    std::vector<int> data;
    std::ranges::for_each(std::views::iota(0zU, n), [&](int) { data.push_back(dis(gen)); });
    std::ranges::shuffle(data, gen);
    std::vector<int> expected;
    std::partial_sum(data.begin(), data.end(), std::back_inserter(expected));

    auto& array = machine.allocate<int>(data);

    std::println("input: {}", str(array));

    machine.parallel(PrefixSumImpl{.array = array});
    std::println("output: {}", str(array));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(array.data == expected, "Prefix sums do not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", machine.n_processors, machine.round_count(),
        machine.read_count(), machine.write_count());
}

int main() try {
    std::println("===== example: prefix_sum =====");
    list_ranking();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

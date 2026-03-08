#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "format.h"  // IWYU pragma: keep

/**
 * 前缀和，CREW 模型，处理器数 O(n)，时间复杂度 O(logn)
 */
struct PrefixSumImpl {
    pram::SharedArray<int>& input;
    pram::SharedArray<int>& output;

    pram::Task operator()(size_t pid) {
        size_t n = input.size();

        output.write(pid, input[pid]);
        co_await pram::barrier();

        for (size_t stride = 1; stride < n; stride *= 2) {
            int temp = 0;
            if (pid >= stride) {
                temp = output[pid] + output[pid - stride];
            }
            co_await pram::barrier();
            if (pid >= stride) {
                output.write(pid, temp);
            }
            co_await pram::barrier();
        }
    }
};

void prefix_sum() {
    constexpr size_t n = 8;

    pram::Machine machine{n, pram::CREW};

    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis(1, 4);
    std::vector<int> data;
    std::ranges::for_each(std::views::iota(0zU, n), [&](int) { data.push_back(dis(gen)); });
    std::ranges::shuffle(data, gen);
    auto& input = machine.allocate<int>(data);
    auto& output = machine.allocate<int>(n);

    std::println("input: {}", input);

    machine.parallel(PrefixSumImpl{.input = input, .output = output});

    std::println("output: {}", output);
    std::vector<int> prefix_sums;
    std::partial_sum(data.begin(), data.end(), std::back_inserter(prefix_sums));
    pram::assert_or_throw(output.data == prefix_sums, "Prefix sums do not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", machine.n_processors, machine.round_count(),
        machine.read_count(), machine.write_count());
}

int main() try {
    std::println("===== example: prefix_sum =====");
    prefix_sum();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "format.h"  // IWYU pragma: keep

struct BionicSortImpl {
    pram::SharedArray<int>& input;
    pram::SharedArray<int>& output;

    /// 双调排序变种，可以优雅地解决非 2 的幂次规模
    pram::Task operator()(size_t pid) {
        size_t n = input.size();
        output.write(pid, input[pid]);
        co_await pram::barrier();

        for (size_t k = 2; k <= std::bit_ceil(n); k *= 2) {
            for (size_t i = k / 2; i > 0; i /= 2) {
                size_t partner = pid ^ i;
                if (i == k / 2) {
                    partner = pid ^ (k - 1);
                }

                int val_self = 0;
                int val_partner = 0;
                if (pid < partner && partner < n) {
                    val_self = output[pid];
                    val_partner = output[partner];
                }
                co_await pram::barrier();

                if (pid < partner && partner < n) {
                    bool should_swap = val_self > val_partner;

                    if (should_swap) {
                        output.write(pid, val_partner);
                        output.write(partner, val_self);
                    }
                }
                co_await pram::barrier();
            }
        }
    }
};

void bionic_sort() {
    constexpr size_t n = 12;

    pram::Machine machine{n, pram::CREW};

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(n + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto& input = machine.allocate<int>(data);
    auto& output = machine.allocate<int>(n);

    std::println("input: {}", input);

    machine.parallel(BionicSortImpl{.input = input, .output = output});

    std::println("output: {}", output);
    std::ranges::sort(data);
    pram::assert_or_throw(output.data == data, "Sorted output does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", machine.n_processors, machine.round_count(),
        machine.read_count(), machine.write_count());
}

int main() try {
    std::println("===== example: bionic_sort =====");
    bionic_sort();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

#include <pramsim/pramsim.hpp>
#include <print>
#include <random>
#include <ranges>

#include "str.hpp"

/**
 * 双调排序变种，CREW 模型，处理器数 O(n)，时间复杂度 O(log^2{n})
 * 这个变种可以优雅处理非 2 的幂次的输入规模。
 * n = 8 的排序网络如下所示：
 * 0: --#--------#----#--------#--------#----#----
 * 1: --#--------|-#--#--------|-#------|-#--#----
 * 2: ----#------|-#----#------|-|-#----#-|----#--
 * 3: ----#------#------#------|-|-|-#----#----#--
 * 4: --#--------#----#--------|-|-|-#--#----#----
 * 5: --#--------|-#--#--------|-|-#----|-#--#----
 * 6: ----#------|-#----#------|-#------#-|----#--
 * 7: ----#------#------#------#----------#----#--
 */
std::pair<std::vector<int>, pram::Stat> bitonic_sort_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n, pram::CREW};

    auto& array = machine.allocate<int>(data);

    machine.parallel([&](size_t pid) -> pram::Task {
        size_t n = array.size();

        for (size_t k = 2; k <= std::bit_ceil(n); k *= 2) {
            for (size_t i = k / 2; i > 0; i /= 2) {
                size_t partner = pid ^ i;
                if (i == k / 2) {
                    partner = pid ^ (k - 1);
                }

                int val_self = 0;
                int val_partner = 0;
                if (pid < partner && partner < n) {
                    val_self = array[pid];
                    val_partner = array[partner];
                }
                co_await pram::barrier();

                if (pid < partner && partner < n) {
                    if (val_self > val_partner) {
                        array.write(pid, val_partner);
                        array.write(partner, val_self);
                    }
                }
                co_await pram::barrier();
            }
        }
    });

    return {array.data, machine.stat()};
}

void bitonic_sort_example() {
    constexpr size_t n = 12;

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(n + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto expected = data;
    std::ranges::sort(expected);

    std::println("input: {}", str(data));

    auto [result, stat] = bitonic_sort_impl(data);

    std::println("output: {}", str(result));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(result == expected, "The result does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", stat.n_processors, stat.n_rounds, stat.n_reads,
        stat.n_writes);
}

int main() try {
    std::println("===== example: bionic_sort =====");
    bitonic_sort_example();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

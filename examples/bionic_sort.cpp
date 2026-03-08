#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "str.h"

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
struct BionicSortImpl {
    pram::SharedArray<int>& array;

    pram::Task operator()(size_t pid) {
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
    }
};

void bionic_sort() {
    constexpr size_t n = 12;

    pram::Machine machine{n, pram::CREW};

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(n + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto expected = data;
    std::ranges::sort(expected);

    auto& array = machine.allocate<int>(data);

    std::println("input: {}", str(array));

    machine.parallel(BionicSortImpl{.array = array});

    std::println("output: {}", str(array));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(array.data == expected, "The result does not match expected values.");
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

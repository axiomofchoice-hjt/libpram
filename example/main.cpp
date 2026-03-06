#include <print>
#include <random>
#include <ranges>

#include "pram/pram.h"

void print(const pram::SharedArray<int>& array) {
    std::print("[");
    for (size_t i = 0; i < array.data.size(); ++i) {
        if (i != 0) {
            std::print(", ");
        }
        std::print("{}", array.data[i]);
    }
    std::print("]\n");
}

void parallel_sort() {
    pram::Machine machine{pram::CRCW_Add};

    constexpr size_t size = 8;

    std::mt19937 gen{std::random_device{}()};
    auto data = std::views::iota(1, static_cast<int>(size + 1)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(data, gen);
    auto& array = machine.allocate<int>(std::move(data));
    auto& rank = machine.allocate<size_t>(size);

    print(array);

    machine.parallel(size * size, [&](size_t pid) -> pram::Task {
        size_t i = pid / size;
        size_t j = pid % size;
        int value_i = array[i];
        int value_j = array[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);
        }
        co_await machine.barrier();
    });

    machine.parallel(size, [&](size_t pid) -> pram::Task {
        size_t index = rank[pid];
        int value = array[pid];
        array.write(index, value);
        co_await machine.barrier();
    });

    print(array);
}

int main() try { parallel_sort(); } catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
} catch (const std::exception& e) {
    std::println("Exception: {}", e.what());
    return 1;
}

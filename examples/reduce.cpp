#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "format.h"  // IWYU pragma: keep

void reduce() {
    constexpr size_t size = 8;

    pram::Machine machine{size, pram::EREW};

    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dis(1, 4);
    std::vector<int> data;
    std::ranges::for_each(std::views::iota(0zU, size), [&](int) { data.push_back(dis(gen)); });
    std::ranges::shuffle(data, gen);
    auto& input = machine.allocate<int>(data);
    auto& output = machine.allocate<int>(1);
    auto& buffer = machine.allocate<int>(size);

    std::println("input: {}", input);

    machine.parallel([&](size_t pid) -> pram::Task {
        buffer.write(pid, input[pid]);
        co_await pram::barrier();

        for (size_t stride = 1; stride < size; stride *= 2) {
            int value = 0;
            if (pid % (2 * stride) == 0 && pid + stride < size) {
                value = buffer[pid] + buffer[pid + stride];
            }
            co_await pram::barrier();
            if (pid % (2 * stride) == 0 && pid + stride < size) {
                buffer.write(pid, value);
            }
            co_await pram::barrier();
        }

        if (pid == 0) {
            output.write(0, buffer[0]);
        }
    });

    std::println("output: {}", output);
    int expected = std::ranges::fold_left(input.data, 0, std::plus{});
    pram::assert_or_throw(output.data[0] == expected, "Reduce does not match expected value.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", machine.n_processors, machine.round_count(),
        machine.read_count(), machine.write_count());
}

int main() try {
    std::println("===== example: reduce =====");
    reduce();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

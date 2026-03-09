#include <pramsim/pramsim.hpp>
#include <print>
#include <random>
#include <ranges>

#include "str.hpp"

/**
 * List Ranking，CREW 模型，处理器数 O(n)，时间复杂度 O(logn)
 */
std::pair<std::vector<size_t>, pram::Stat> list_ranking_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n, pram::CREW};

    auto& next = machine.allocate<int>(data);
    auto& dist = machine.allocate<size_t>(n);

    machine.parallel([&](size_t pid) -> pram::Task {
        size_t n = next.size();

        int next_id = next[pid];
        co_await pram::barrier();
        dist.write(pid, next_id == -1 ? 0 : 1);
        co_await pram::barrier();

        for (size_t stride = 1; stride < n; stride *= 2) {
            int next_id = next[pid];
            size_t self_dist = dist[pid];
            int next_next_id = -1;
            size_t next_dist = 0;
            if (next_id != -1) {
                next_next_id = next[next_id];
                next_dist = dist[next_id];
            }
            co_await pram::barrier();

            if (next_id != -1) {
                dist.write(pid, self_dist + next_dist);
                next.write(pid, next_next_id);
            }
            co_await pram::barrier();
        }
    });

    return {dist.data, machine.stat()};
}

void list_ranking_example() {
    constexpr size_t n = 8;

    std::mt19937 gen{std::random_device{}()};
    std::vector<int> data(n, -1);
    auto perm = std::views::iota(0, static_cast<int>(n)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(perm, gen);
    std::vector<size_t> expected(n, 0zU);
    std::ranges::for_each(std::views::iota(0zU, n - 1), [&](size_t i) {
        data[perm[i]] = perm[i + 1];
        expected[perm[i]] = n - 1 - i;
    });

    std::println("next: {}", str(data));

    auto [result, stat] = list_ranking_impl(data);

    std::println("result: {}", str(result));
    std::println("expected: {}", str(expected));
    pram::assert_or_throw(result == expected, "The result does not match expected values.");
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", stat.n_processors, stat.n_rounds, stat.n_reads,
        stat.n_writes);
}

int main() try {
    std::println("===== example: list_ranking =====");
    list_ranking_example();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

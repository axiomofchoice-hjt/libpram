#include <pramsim/pramsim.h>

#include <print>
#include <random>
#include <ranges>

#include "format.h"  // IWYU pragma: keep

/**
 * List Ranking，CREW 模型，处理器数 O(n)，时间复杂度 O(logn)
 */
struct ListRankingImpl {
    pram::SharedArray<int>& next;
    pram::SharedArray<size_t>& dist;

    pram::Task operator()(size_t pid) {
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
    }
};

void list_ranking() {
    constexpr size_t n = 8;

    pram::Machine machine{n, pram::CREW};

    std::mt19937 gen{std::random_device{}()};
    std::vector<int> data(n, -1);
    auto perm = std::views::iota(0, static_cast<int>(n)) | std::ranges::to<std::vector>();
    std::ranges::shuffle(perm, gen);
    std::ranges::for_each(std::views::iota(0zU, n - 1), [&](size_t i) { data[perm[i]] = perm[i + 1]; });
    auto& next = machine.allocate<int>(data);
    auto& dist = machine.allocate<size_t>(n);

    std::println("next: {}", next);

    machine.parallel(ListRankingImpl{.next = next, .dist = dist});

    std::println("dist: {}", dist);
    std::println("n_processors: {}, rounds: {}, reads: {}, writes: {}", machine.n_processors, machine.round_count(),
        machine.read_count(), machine.write_count());
}

int main() try {
    std::println("===== example: list_ranking =====");
    list_ranking();
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}

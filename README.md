# pramsim - A PRAM simulator for parallel algorithm experiments

pramsim 是一个 PRAM（Parallel Random Access Machine）模拟器，用于并行复杂度实验与模型验证。

特性：现代 C++ / Header Only / 协程。

## 1. 使用 xmake 构建

要求支持 C++23 的编译器（clang / gcc）。

```bash
xmake config -m release
xmake
```

等待构建完成，执行命令 `xmake run` 就能运行 examples 的程序。

## 2. 命令行构建

```bash
g++ examples/rank_sort.cpp -I src -std=c++23
```

等待构建完成，执行命令 `./a.out` 就能运行 rank_sort。

## 3. 示例

使用 CRCW_Add PRAM 进行排序，$O(n^2)$ 个处理器，$O(1)$ 时间复杂度。

完整示例见 [examples/rank_sort.cpp](examples/rank_sort.cpp)。

```cpp
struct RankSortImpl {
    pram::SharedArray<int>& input;
    pram::SharedArray<size_t>& rank;
    pram::SharedArray<int>& output;

    pram::Task operator()(size_t pid) {
        size_t n = input.size();
        size_t i = pid / n;
        size_t j = pid % n;

        int value_i = input[i];
        int value_j = input[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);
        }
        co_await pram::barrier();
        if (j == 0) {
            output.write(rank[i], value_i);
        }
    }
};
```

## 4. 未实现的特性

- 随机的 arbitrary CRCW。
- priority CRCW。

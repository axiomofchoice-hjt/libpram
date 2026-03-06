# libpram

libpram 是 C++ Header Only 库，实现了一个 PRAM（Parallel Random Access Machine）运行时，用于并行复杂度实验与模型验证。

## 1. 使用 xmake 构建

要求支持 C++23 的编译器（clang / gcc）。

```bash
xmake config -m release
xmake
```

命令 `xmake run` 可以运行 example 的程序。

## 2. 命令行构建

```bash
g++ example/main.cpp -I src -std=c++23
```

命令 `./a.out` 可以运行 example 的程序。

## 3. 示例

使用 CRCW_Add PRAM 进行 $O(1)$ 排序。

完整示例见 example/main.cpp: parallel_sort。

```cpp
pram::Machine machine{pram::CRCW_Add};

constexpr size_t size = 8;

std::mt19937 gen{std::random_device{}()};
auto data = std::views::iota(1, static_cast<int>(size + 1)) | std::ranges::to<std::vector>();
std::ranges::shuffle(data, gen);
auto& array = machine.allocate<int>(std::move(data));
auto& rank = machine.allocate<size_t>(size);

machine.parallel(size * size, [&](size_t pid) -> pram::Task {
    size_t i = pid / size;
    size_t j = pid % size;
    int value_i = array[i];
    int value_j = array[j];
    if (std::pair{value_i, i} > std::pair{value_j, j}) {
        rank.write(i, 1);
    }
    co_await pram::barrier();
    if (j == 0) {
        array.write(rank[i], value_i);
    }
});
```

## 4. 未实现的特性

- 随机的 arbitrary CRCW。
- priority CRCW。

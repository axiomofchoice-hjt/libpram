# libpram

libpram 是一个基于 C++ 实现的 PRAM（Parallel Random Access Machine）运行时库，用于并行复杂度实验与模型验证。

## 1. 使用 xmake 构建

要求支持 C++23 的编译器（clang / gcc）。

```bash
xmake config -m release
xmake
```

命令 `xmake run` 可以运行 example 的程序。

## 2. 示例

使用 CRCW_Add PRAM 进行 $O(1)$ 排序。目前同步机制是，每次 read / write 会触发全局同步。

完整示例见 example/main.cpp: parallel_sort。

```cpp
pram::Machine machine{pram::CRCW_Add};

constexpr size_t size = 8;

std::mt19937 gen{std::random_device{}()};
auto data = std::views::iota(1, static_cast<int>(size + 1)) | std::ranges::to<std::vector>();
std::ranges::shuffle(data, gen);
auto& array = machine.allocate<int>(std::move(data));
auto& counter = machine.allocate<size_t>(size);

machine.parallel(size * size, [&](size_t pid) -> pram::Task {
    size_t i = pid / size;
    size_t j = pid % size;
    int value_i = co_await array.read(i);
    int value_j = co_await array.read(j);
    if (std::pair{value_i, i} > std::pair{value_j, j}) {
        co_await counter.write(i, 1);
    }
});

machine.parallel(size, [&](size_t pid) -> pram::Task {
    size_t index = co_await counter.read(pid);
    int value = co_await array.read(pid);
    co_await array.write(index, value);
});
```

## 3. 未实现的特性

- EREW, CREW 读和写冲突检查。
- 随机的 arbitrary CRCW。
- priority CRCW。

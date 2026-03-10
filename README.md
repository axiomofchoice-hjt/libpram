# pramsim - A PRAM simulator for parallel algorithm experiments

pramsim 是一个 PRAM（Parallel Random Access Machine）模拟器，用于并行复杂度实验与模型验证。

## 1. 特性

- C++23 / Header Only / 协程
- 教学友好

## 2. 快速开始

### 2.1. 准备

- 支持 C++23 的编译器 (gcc / clang)
- （可选）xmake 构建工具

### 2.2. 获取源码

可以用 git clone 命令获取：

```bash
git clone https://github.com/axiomofchoice-hjt/pramsim.git
```

### 2.3. 使用 xmake 构建（推荐）

进入项目目录，执行构建命令：

```bash
xmake

# 或者指定构建模式
# xmake config -m release && xmake
```

等待构建完成后，执行命令运行 `examples` 的所有程序：

```bash
xmake run
```

要安装 pramsim 库，执行命令：

```bash
xmake install pramsim

# 或者指定安装路径
# xmake install --installdir=/path/to/install pramsim
```

### 2.4. 直接编译

执行命令：

```bash
g++ examples/rank_sort.cpp -DPRAMSIM_STANDALONE -Iinclude -std=c++23 -o rank_sort
```

等待构建完成后，执行命令运行 `rank_sort`：

```bash
./rank_sort
```

## 3. 示例

使用 CRCW_Add PRAM 进行排序，$O(n^2)$ 个处理器，$O(1)$ 时间复杂度。

完整示例见 [examples/rank_sort.cpp](examples/rank_sort.cpp)。

```cpp
std::pair<std::vector<int>, pram::Stat> rank_sort_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n * n, pram::CRCW_Add};

    auto& array = machine.allocate<int>(data);
    auto& rank = machine.allocate<size_t>(n);

    machine.parallel([&](size_t pid) -> pram::Task {
        size_t i = pid / n;
        size_t j = pid % n;

        int value_i = array[i];
        int value_j = array[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);
        }
        co_await pram::step();
        if (j == 0) {
            array.write(rank[i], value_i);
        }
    });

    return {array.data, machine.stat()};
}
```

# pramsim - A PRAM simulator for parallel algorithm experiments

pramsim 是一个 PRAM（Parallel Random Access Machine）模拟器，用于并行复杂度实验、模型验证和教学演示。

特性：

1. 现代构建系统和语法：Xmake，C++23，Coroutine。
2. 轻量级，Header Only。
3. 教学友好，易用，完备的 PRAM 模型支持。

## 1. PRAM 是什么

PRAM 是一种用于并行算法研究的常见理论模型。在该模型中，大量处理器同步执行，并共享一块随机访问内存。

计算按轮 (round) 进行。每一轮中，处理器可以执行计算和读写内存，这一轮结束后会进行全局同步，然后进入下一轮。也就是说 PRAM 是天然同步的。

根据读写同一内存地址的规则，可以对 PRAM 进行分类：

1. EREW (Exclusive Read Exclusive Write)：同一地址不能同时读或写。
2. CREW (Concurrent Read Exclusive Write)：同一地址可以同时读，不能同时写。
3. CRCW (Concurrent Read Concurrent Write)：同一地址可以同时读和写。对于写冲突有额外的策略，例如 CRCW_Add 对同一地址的并发写会自动求和。

分析并行算法会有两个指标：时间复杂度和处理器数量。例如 $`O(log n)`$ 时间、$`O(n)`$ 处理器。

## 2. 示例

使用 CRCW_Add PRAM 进行排序，$`O(1)`$ 时间，$`O(n^2)`$ 处理器。

方法为两两比较数组里的元素（正好用到 $`O(n^2)`$ 个处理器），利用 CRCW_Add 并发写的自动求和机制，算出每个元素的排名。然后根据排名把元素写到数组对应位置。

完整示例见 [examples/rank_sort.cpp](examples/rank_sort.cpp)。

```cpp
std::pair<std::vector<int>, pram::Stat> rank_sort_impl(const std::vector<int>& data) {
    size_t n = data.size();
    pram::Machine machine{n * n, pram::CRCW_Add};

    auto& array = machine.allocate<int>(data);
    auto& rank = machine.allocate<size_t>(n, 0);

    machine.parallel([&](size_t pid) -> pram::Task {
        size_t i = pid / n;
        size_t j = pid % n;

        int value_i = array[i];
        int value_j = array[j];
        if (std::pair{value_i, i} > std::pair{value_j, j}) {
            rank.write(i, 1);  // 在 CRCW_Add 模型下表示并发写入求和
        }
        co_await pram::step();
        if (j == 0) {
            array.write(rank[i], value_i);
        }
    });

    return {array.data, machine.stat()};
}
```

## 3. 快速开始

### 3.1. 准备

- 支持 C++23 的编译器 (gcc / clang)
- （可选）xmake 构建工具

### 3.2. 使用 xmake 构建（推荐）

获取源码，可以用 git clone 命令获取：

```bash
git clone https://github.com/axiomofchoice-hjt/pramsim.git
```

进入项目目录，执行构建命令：

```bash
xmake

# 也可以指定构建模式
# xmake config -m release && xmake
```

等待构建完成后，执行命令运行 `examples` 的所有程序：

```bash
xmake run
```

要安装 pramsim 库，执行命令：

```bash
xmake install pramsim

# 也可以指定安装路径
# xmake install --installdir=/path/to/install pramsim
```

### 3.3. 直接编译

获取源码，可以用 git clone 命令获取：

```bash
git clone https://github.com/axiomofchoice-hjt/pramsim.git
```

进入项目目录，执行编译命令：

```bash
g++ examples/rank_sort.cpp -DPRAMSIM_STANDALONE -Iinclude -std=c++23 -o rank_sort
```

等待编译完成后，执行命令运行 `rank_sort`：

```bash
./rank_sort
```

## 4. API

### 4.1. 包含头文件

```cpp
#include <pramsim/pramsim.hpp>
```

### 4.2. Machine

Machine 表示一台 PRAM 机器。

### 4.3. SharedArray

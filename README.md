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

分析并行算法会有两个指标：时间复杂度和处理器数量。例如 $`O(\log n)`$ 时间、$`O(n)`$ 处理器。

## 2. 示例

数组的循环右移：程序会启动 n 个处理器，每个处理器读取一个数组元素，然后在下一轮把它写到右边的位置。

`co_await pram::step();` 表示 round barrier。这一轮的写操作会统一提交，并进行读写冲突检测。

```cpp
#include <pramsim/pramsim.hpp>
#include <vector>

int main() {
    constexpr size_t n = 8;
    pram::Machine machine{n, pram::EREW};
    auto& array = machine.allocate<int>(std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7});

    machine.parallel([&](size_t pid) -> pram::Task {
        int value = array[pid];
        co_await pram::step();
        array.write((pid + 1) % n, value);
    });
}
```

更多示例：

1. `examples/tree_sum.cpp` - 数组求和
2. `examples/prefix_sum.cpp` - 前缀和
3. `examples/rank_sort.cpp` - Rank Sort
4. `examples/bitonic_sort.cpp` - 双调排序（变种）
5. `examples/list_ranking.cpp` - 链表排名

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
g++ examples/rank_sort.cpp -Iinclude -std=c++23 -o rank_sort
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

# user guide

## 1. 基本结构

1. 创建一个 Machine。
2. 用 `machine.allocate(...)` 创建若干个 SharedArray。
3. 用 `machine.parallel(...)` 启动并行程序。

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

## 2. 模型类型

PRAM 模型描述了读写同一地址的策略。可以用 `pram::CREW` 等获取对应常量，传给 Machine 构造函数的第二个参数。

pramsim 支持的模型如下：

|      模型      | 读策略 |    写策略    |        说明        |
| :------------: | :----: | :----------: | :----------------: |
|      EREW      |  互斥  |    互斥写    |     最严格模型     |
|      CREW      |  并发  |    互斥写    |      常用模型      |
|  CRCW_Common   |  并发  |    公共写    |  只能同时写相同值  |
| CRCW_Arbitrary |  并发  |    任意写    |   随机写入一个值   |
| CRCW_Priority  |  并发  |   优先级写   | 编号小的处理器优先 |
|    CRCW_Add    |  并发  |  合并写(加)  |       值累加       |
|    CRCW_Max    |  并发  | 合并写(最大) |      取最大值      |
|    CRCW_Min    |  并发  | 合并写(最小) |      取最小值      |

## 3. 读写操作

共享数组 SharedArray 提供读写操作：

- `array[index]` 读 index 位置的内存，立即生效。
- `array.write(index, value)` 将 value 写入 index 位置的内存，但是不立即生效，到同步点才会生效。

注意 SharedArray 提供 `array._data` 成员变量，仅用于验证、调试使用，对这个变量读写会绕过 PRAM 检查。

## 4. 同步点

C++ 协程提供了优雅的暂停函数方法，使用 co_await 暂停这个协程：

```cpp
co_await pram::step();
```

所有处理器执行到同步点后，会进行：

1. 读写冲突检测，pramsim 根据模型类型检查程序行为是否合法。
2. 提交写操作，调用 `array.write` 不会立即写入，只会在同步点进行写入。

## 5. 调试输出

示例：

```cpp
machine.parallel([&](size_t pid) -> pram::Task {
    array.write(pid, pid);
    co_await pram::step();
    if (pid == 0) {
        std::println("debug: {}", str(array._data));
    }
});
```

如上所示，为了查看写操作生效之后的值，建议在同步点 `co_await pram::step();` 的后面打印，`if (pid == 0)` 保证只打印一次。

`str` 的实现位于 `examples/str.hpp`。

## 6. 错误排查

pramsim 内部可能抛出 `pram::assertion_error` 异常，在外层要用 try catch 获取。例如：

```cpp
int main() try {
    /* ... */
} catch (const pram::assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
}
```

异常信息可能是：

- `Read-write conflict: read and write to the same address` 同时对一个地址进行读和写。
- `Read conflict: exclusive read to the same address` 仅限 EREW 模型，不同处理器同时读同一地址。
- `Write conflict: exclusive write to the same address` EREW 或 CREW 模型，不同处理器同时写同一地址。
- `Write conflict: common write with different values` 仅限 CRCW_Common 模型，不同处理器同时用不同值写同一地址。
- `Read outside parallel region` 在 parallel 范围外读共享内存。
- `Write outside parallel region` 在 parallel 范围外写共享内存。

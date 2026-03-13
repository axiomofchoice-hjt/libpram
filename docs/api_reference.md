# api reference

## 1. Machine

Machine 是 PRAM 机器，用于管理共享内存、上下文、调度。

### 1.1. Machine::Machine

```cpp
Machine::Machine(size_t n_processors);               // (1)
Machine::Machine(size_t n_processors, Model model);  // (2)
```

(1) 构造一个 n_processors 个处理器，模型类型为 CREW 的 PRAM 机器。

(2) 构造一个 n_processors 个处理器，模型类型为 model 的 PRAM 机器。

模型类型见 user_guide.md。

### 1.2. Machine::allocate

```cpp
SharedArray<T>& Machine::allocate(size_t length, T value);  // (1)
SharedArray<T>& Machine::allocate(std::vector<T> data);     // (2)
```

(1) 分配一个长度为 length 元素初值为 value 的共享数组。

(2) 分配一个初值为 data 的共享数组。

共享数组见 [SharedArray](#2-sharedarray)。

### 1.3. Machine::parallel

```cpp
template <std::invocable<size_t> F>
void Machine::parallel(this auto&& self, F&& func);
```

模拟 PRAM 并行程序。func 是一个协程，参数为处理器 id，返回值为 `pram::Task`。

PRAM 机器会启动 n_processors 个处理器，它们的 id 为 0 到 n_processors - 1。

func 内部用可以 `co_await pram::step();` 进行同步。

### 1.4. Machine::stat

```cpp
Stat Machine::stat() const;
```

返回一些统计信息，包括处理器数 n_processors，轮数 n_rounds，读内存数 n_reads，写内存数 n_writes。

Stat 的定义如下：

```cpp
struct Stat {
    size_t n_processors;
    size_t n_rounds;
    size_t n_reads;
    size_t n_writes;
};
```

## 2. SharedArray

SharedArray 是共享数组，用于检测读写冲突、提交写操作。

### 2.1. operator[]

```cpp
template <typename T>
T SharedArray<T>::operator[](size_t index);
```

读 index 位置的内存。

### 2.2. SharedArray::write

```cpp
template <typename T>
void SharedArray<T>::write(size_t index, T value);
```

将 value 写入 index 位置的内存。

### 2.3. SharedArray::size

```cpp
template <typename T>
size_t SharedArray<T>::size() const;
```

返回共享数组的大小。

### 2.4. SharedArray::_data

```cpp
template <typename T>
std::vector<T> SharedArray<T>::_data;
```

成员变量，仅用于验证、调试使用，对这个变量读写会绕过 PRAM 检查。

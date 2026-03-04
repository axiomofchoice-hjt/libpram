# libpram - PRAM Runtime Library

libpram 是一个基于 C++ 实现的 PRAM（Parallel Random Access Machine）运行时库，用于并行复杂度实验与模型验证。

## 构建

项目使用 **xmake**，要求支持 C++23 的编译器（clang / gcc）。

```bash
xmake config -m release
xmake
```

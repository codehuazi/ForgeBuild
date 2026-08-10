# ForgeBuild

ForgeBuild 是一个用于学习构建系统核心原理的 C++ 项目。

当前目标是逐步实现：

- 构建描述解析
- 文件依赖图
- 增量构建判断
- 并行任务调度
- 命令执行
- 编译缓存

## 当前进度

Day2 已完成核心构建图模型：

- `Rule`：描述通用构建方式
- `Node`：表示构建资源
- `Edge`：表示一次具体构建关系
- `BuildGraph`：拥有并维护完整依赖图

当前示例图：

```text
main.cpp --compile--> main.o --\
                                link --> app
math.cpp --compile--> math.o --/

构建
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
运行
./build/debug/graph_demo
# ForgeBuild

[![CI](https://github.com/codehuazi/ForgeBuild/actions/workflows/ci.yml/badge.svg)](https://github.com/codehuazi/ForgeBuild/actions/workflows/ci.yml)

ForgeBuild 是一个基于 **C++20 / Linux** 实现的增量并行构建引擎。

项目围绕现代构建系统的核心问题展开，实现了 **依赖 DAG、最小增量重建、并行任务调度、GCC/Clang 动态头文件依赖追踪以及本地内容寻址缓存**，并通过 CTest、Sanitizer、Benchmark 和 GitHub Actions 对正确性、性能和可靠性进行验证。

---

## Features

* **Manifest 解析**

  * 支持 `rule` / `build`
  * 支持 `$in`、`$out` 命令展开
  * 构建 `Rule / Node / Edge / BuildGraph` 模型

* **依赖 DAG**

  * 使用有向图描述构建任务依赖
  * 支持拓扑关系分析与环检测
  * 根据依赖关系生成可执行任务序列

* **增量构建**

  * 基于输入 / 输出时间戳判断 Dirty 状态
  * Dirty 状态向下游传播
  * 只生成本次真正受影响的 `BuildPlan`
  * 构建命令变化时自动失效

* **动态 Header 依赖**

  * 解析 GCC / Clang `-MMD -MF` 生成的 depfile
  * 使用 `DepsLog` 持久化 Header 依赖
  * Header 修改或删除后自动触发相关 Translation Unit 重建

* **并行 DAG 调度**

  * 支持 `-j N`
  * Worker Thread + Ready Queue
  * 根据依赖完成情况动态释放下游任务
  * 支持失败传播与 Worker 正常退出

* **多输出 Edge**

  * 单个构建任务可以生成多个 Output
  * 避免同一个 Edge 被重复调度执行

* **内容寻址本地缓存**

  * 基于 Command + Dependency Path + Dependency Content 构造 Cache Key
  * 使用 64-bit FNV-1a 流式 Hash
  * 支持 Cache Store / Restore
  * 使用临时文件 + Rename 提交缓存对象
  * Metadata 校验 Size / Content Hash
  * 缓存损坏时自动失效并回退正常编译

---

## Architecture

```text
build.forge
    |
    v
Manifest Parser
    |
    v
BuildGraph
    |
    v
Dirty Analysis
    |
    v
BuildPlanner
    |
    v
Affected BuildPlan
    |
    v
Parallel Scheduler
    |
    +----------+----------+----------+
    |          |          |          |
    v          v          v          v
 Worker     Worker     Worker     Worker
    |          |          |          |
    +----------+----------+----------+
                   |
                   v
                Executor
                   |
            +------+------+
            |             |
            v             v
        Cache Hit      Run Command
                          |
                          v
                  Depfile / DepsLog
                          |
                          v
                       BuildLog
```

核心模块职责：

| Module         | Responsibility                |
| -------------- | ----------------------------- |
| `BuildGraph`   | 管理 `Rule / Node / Edge` 及依赖关系 |
| `Builder`      | 判断 Dirty 状态并传播影响              |
| `BuildPlanner` | 生成最小受影响 BuildPlan             |
| `Scheduler`    | 根据 DAG 依赖并行调度任务               |
| `Executor`     | 执行命令并维护日志、动态依赖和缓存             |
| `DepsLog`      | 持久化 Header 动态依赖               |
| `LocalCache`   | 保存和恢复内容寻址构建结果                 |

---

## Quick Start

### Requirements

* Linux
* C++20
* CMake
* GCC / Clang
* pthread

### Build ForgeBuild

```bash
cmake \
    -S . \
    -B build/debug \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON

cmake \
    --build build/debug \
    --parallel
```

生成：

```text
build/debug/forge
```

### Run Example

仓库根目录提供了一个简单的 C++ 示例工程：

```text
a.cpp ----> a.o ---\
                    \
b.cpp ----> b.o -----+--> app
                    /
main.cpp -> main.o -/
```

对应 `build.forge`：

```text
rule compile
 command = g++ -std=c++20 -MMD -MF $out.d -c $in -o $out
 depfile = $out.d

rule link
 command = g++ $in -o $out

build a.o: compile a.cpp
build b.o: compile b.cpp
build main.o: compile main.cpp
build app: link a.o b.o main.o
```

使用 3 个 Worker：

```bash
./build/debug/forge -j 3 build.forge
```

运行结果：

```bash
./app
```

```text
30
```

---

## Incremental Build

ForgeBuild 不会简单地在每次运行时重新执行整个构建图。

例如只修改：

```text
a.cpp
```

本次受影响路径为：

```text
a.cpp
  |
  v
a.o
  |
  v
app
```

因此只需要执行：

```text
Compile a.cpp
+
Link app
```

与变化无关的 `b.cpp` 不会重新编译。

### Dynamic Header Dependency

Compile Rule 使用：

```text
-MMD -MF $out.d
```

生成：

```text
main.o: main.cpp a.hpp b.hpp
```

ForgeBuild 将这些依赖写入 `DepsLog`。

因此 Header 即使没有显式写入 Manifest，修改：

```text
a.hpp
```

后仍然能够自动找到真正受影响的编译任务。

---

## Parallel Build Benchmark

Benchmark 包含：

```text
32 C++ Translation Units
+
1 Link Edge
```

测试环境：

```text
Ubuntu VM
4 vCPU
GCC
```

Cold Build 每组运行 5 次：

| Workers | Average | Speedup | Parallel Efficiency |
| ------- | ------: | ------: | ------------------: |
| `-j1`   | 3.206 s |   1.00× |                100% |
| `-j2`   | 1.648 s |   1.95× |               97.3% |
| `-j4`   | 1.062 s |   3.02× |               75.5% |

`-j4` 相比 `-j1`：

```text
3.206 s
   ↓
1.062 s
```

获得约 **3.02× Speedup**，构建耗时下降约 **66.9%**。

实际加速不会随 Worker 数量无限线性增长，主要受到最终串行 Link、进程创建、CPU / I/O 竞争和同步开销等因素影响。

---

## Content Cache

当输出文件已经不存在，但此前执行过相同的构建任务时：

```text
Output Missing
      |
      v
Edge enters BuildPlan
      |
      v
Calculate Cache Key
      |
      v
Cache Hit
      |
      v
Restore Output
```

Benchmark 中删除全部 Output、保留有效 Cache 和 DepsLog：

```text
-j4 Cold Build : 1.062 s
Cache Restore  : 0.086 s
```

10 次 Cache Restore：

```text
Average : 0.086 s
Median  : 0.080 s
```

相较 `-j4` Cold Build：

* **约 12.35× Speedup**
* **构建耗时下降约 91.9%**

### Incremental Build vs Cache

**Incremental Build**

```text
Input unchanged
+
Output valid
      |
      v
Skip Edge
```

避免执行不必要任务。

**Content Cache**

```text
Task needs execution
      |
      v
Cache Hit
      |
      v
Restore historical result
```

复用已经计算过的结果。

---

## Minimal Rebuild Validation

### Modify One Source

修改：

```text
benchmark/src/file_00.cpp
```

结果：

```text
planned edge count: 2
```

只执行：

```text
file_00.cpp -> file_00.o
+
link
```

其他 31 个 Compile Edge 不重新执行。

### Modify Shared Header

31 个模块共同依赖：

```text
benchmark/include/common.hpp
```

修改后：

```text
planned edge count: 32
```

执行：

```text
31 Compile Edge
+
1 Link Edge
```

不依赖该 Header 的 `main.cpp` 不会重新编译。

该场景验证了：

* depfile 动态依赖
* Dirty Propagation
* High Fan-Out Dependency
* 最小受影响子图

---

## Testing & Reliability

### CTest

核心回归测试通过 CTest 统一管理：

```bash
ctest \
    --test-dir build/debug \
    --output-on-failure
```

当前注册：

```text
scheduler_guard
scheduler_multi_output
scheduler_failure
hash
local_cache
compiler_identity
incremental_source
incremental_header
incremental_command
```

当前结果：

```text
9 / 9 tests passed
```

测试覆盖包括：

Scheduler 正确性
非法调度状态保护
Multi-Output Edge 去重执行
任务失败向下游传播
Hash / Content Cache
增量 Hash 与 Cache Key
Compiler Identity
Cache Store / Restore
临时文件 + Rename 原子提交
Size / Content Hash 完整性校验
Cache Corruption 自动失效
Incremental Rebuild
修改显式源文件时，只重建受影响 Compile Edge 及下游 Link
修改 Header 时，通过 depfile / DepsLog 找到动态依赖并生成最小受影响 BuildPlan
编译命令变化时，通过 BuildLog 中保存的 Command Hash 使旧产物失效
--explain 验证直接 Dirty Reason 与下游 Dirty Propagation

其中增量回归测试均在独立临时目录中执行真实的 Compile / Link 流程，避免依赖开发目录中的历史构建产物。

---

### Sanitizer

分别建立独立构建配置：

```text
build/sanitize
├── AddressSanitizer
└── UndefinedBehaviorSanitizer

build/tsan
└── ThreadSanitizer
```

主要用于检测：

* **ASan**：非法内存访问、越界等
* **UBSan**：Undefined Behavior
* **TSan**：多线程 Data Race

除 CTest 外，还使用：

```text
4 Scheduler Workers
32 Compile Edges
1 Link Edge
```

进行端到端动态检测。

当前测试覆盖路径中未发现对应 Sanitizer 报错。

> Sanitizer 属于动态分析，因此该结果不等价于证明程序不存在任何 Bug。

---

### GitHub Actions CI

每次：

```text
push
或
pull request
```

自动执行：

```text
Ubuntu 22.04
      |
      v
Checkout
      |
      v
CMake Configure
      |
      v
Build
      |
      v
CTest
```

当前 GitHub Actions CI 已通过。

CI 使用干净 Runner 从源码重新配置和构建项目，避免本地旧 Build Artifact、Cache 或临时文件导致假成功。

---

## Project Structure

```text
ForgeBuild/
├── .github/
│   └── workflows/
│       └── ci.yml
│
├── include/forge/
│   ├── build_graph.hpp
│   ├── build_planner.hpp
│   ├── builder.hpp
│   ├── executor.hpp
│   ├── scheduler.hpp
│   ├── depfile.hpp
│   ├── deps_log.hpp
│   ├── hash.hpp
│   ├── cache_key.hpp
│   └── local_cache.hpp
│
├── src/
│   ├── main.cpp
│   ├── build_graph.cpp
│   ├── build_planner.cpp
│   ├── builder.cpp
│   ├── executor.cpp
│   ├── scheduler.cpp
│   ├── depfile.cpp
│   ├── deps_log.cpp
│   ├── hash.cpp
│   ├── cache_key.cpp
│   └── local_cache.cpp
│
├── tests/
├── benchmark/
├── examples/
├── docs/
│
├── a.cpp
├── a.hpp
├── b.cpp
├── b.hpp
├── main.cpp
├── build.forge
│
├── CMakeLists.txt
├── .gitignore
└── README.md
```

其中：

* `src/main.cpp`：ForgeBuild 自身入口
* 根目录 `a.cpp / b.cpp / main.cpp / build.forge`：Quick Start 示例工程
* `benchmark/`：32 Translation Unit 性能测试工程
* `docs/`：各阶段设计与实现笔记

---

## Design Highlights

### Minimal Rebuild

只重新执行真正受输入变化影响的 Edge，而不是重新构建整个工程。

### Dependency-Aware Parallelism

任务只有在所有必要前驱已经完成后才进入 Ready Queue，保证并行执行仍满足 DAG 依赖约束。

### Cache Correctness First

```text
Cache Hit
   |
   v
Restore

Cache Miss / Corruption
   |
   v
Normal Compile
```

Cache 只影响性能。

缓存失效或损坏不能改变最终构建正确性。

### Reproducible Validation

项目不仅在开发目录中测试，还通过：

```text
Clean Build
+
CTest
+
Sanitizer
+
GitHub Actions
```

验证从干净环境重新构建和测试的能力。

---

## Current Limitations

ForgeBuild 当前定位为 **单机 Linux 构建引擎**，暂未实现：

* Windows Support
* Remote / Distributed Build
* Remote Cache
* Work Stealing
* File Watching
* Sandbox Execution
* 完整 Shell 语义
* CMake Project Generation
* 完整 Ninja Manifest 兼容

这些功能暂不属于当前项目范围。

---

## Documentation

更详细的设计过程、实现思路与实验记录位于：

```text
docs/
```

其中包含从 Build Graph、Parser、Dirty Propagation，到并行 Scheduler、动态依赖、内容寻址缓存和工程化验证等阶段的完整开发笔记。

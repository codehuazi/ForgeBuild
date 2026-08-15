# ForgeBuild

[![CI](https://github.com/codehuazi/ForgeBuild/actions/workflows/ci.yml/badge.svg)](https://github.com/codehuazi/ForgeBuild/actions/workflows/ci.yml)

**ForgeBuild** 是一个基于 **C++20 / Linux** 实现的增量并行构建引擎。

项目围绕现代构建系统的核心问题展开，实现了：

* 依赖 DAG 与环检测
* 最小增量重建
* GCC / Clang 动态 Header 依赖追踪
* 多线程 DAG 调度
* 构建命令变化失效
* 本地内容寻址缓存
* Dirty Reason 诊断
* CTest / Sanitizer / GitHub Actions 工程化验证

## Highlights

| Item                | Result                             |
| ------------------- | ---------------------------------- |
| Language / Platform | C++20 / Linux                      |
| Parallel Build      | `-j N` dependency-aware scheduler  |
| Benchmark Scale     | 32 Translation Units + 1 Link Edge |
| Parallel Speedup    | `-j4` vs `-j1`: **3.02×**          |
| Cache Restore       | vs `-j4` cold build: **12.35×**    |
| Regression Tests    | **9 / 9 passed**                   |
| CI                  | GitHub Actions                     |

在 4 vCPU Ubuntu VM 的 32 Translation Unit Benchmark 中：

```text
-j1 Cold Build : 3.206 s
-j2 Cold Build : 1.648 s
-j4 Cold Build : 1.062 s
Cache Restore  : 0.086 s
```

其中 `-j4` 相比 `-j1` 获得约 **3.02×** 并行加速；有效 Cache 下恢复构建结果相比 `-j4` Cold Build 获得约 **12.35×** 加速。

---

## Core Features

### Incremental Build

ForgeBuild 根据输入、输出以及历史构建信息判断 Edge 是否 Dirty，并只生成真正需要执行的最小 `BuildPlan`。

当前覆盖的主要失效条件包括：

* Output 不存在
* Input 时间戳晚于 Output
* 动态 Header 依赖变化
* 构建命令发生变化
* 上游 Edge 已经 Dirty

例如：

```text
a.cpp ----> a.o ---\
                    \
b.cpp ----> b.o -----+--> app
                    /
main.cpp -> main.o -/
```

仅修改 `a.cpp` 后：

```text
a.cpp
  |
  v
a.o
  |
  v
app
```

只重新执行：

```text
Compile a.cpp
Link app
```

`b.o` 和 `main.o` 保持复用。

### Dynamic Header Dependencies

Compile Rule 支持 GCC / Clang depfile：

```text
-MMD -MF $out.d
```

例如编译器生成：

```text
main.o: main.cpp a.hpp b.hpp
```

ForgeBuild 解析 depfile，并通过 `DepsLog` 持久化这些运行时发现的 Header 依赖。

因此即使 Header 没有显式写在 Manifest 中：

```text
a.hpp changed
      |
      +--> a.o
      |
      +--> main.o
              |
              v
             app
```

仍然能够找到真正受影响的 Translation Unit，而不会重新构建整个工程。

### Parallel DAG Scheduler

ForgeBuild 支持：

```bash
-j N
```

Scheduler 使用：

```text
Dependency DAG
      |
      v
Initial Ready Edges
      |
      v
  Ready Queue
      |
  +---+---+---+
  |   |   |   |
 W1  W2  W3  W4
  |   |   |   |
  +---+---+---+
      |
      v
Release Downstream Edges
```

核心机制包括：

* Worker Thread + Ready Queue
* 根据前驱完成状态动态释放任务
* 保证 DAG 依赖约束
* Failure Propagation
* Multi-Output Edge 去重
* Worker 正常退出

### Content-Addressed Local Cache

当某个任务需要执行时，Executor 会先计算 Cache Key：

```text
Command
+
Dependency Paths
+
Dependency Contents
        |
        v
     Cache Key
```

随后：

```text
Need Build
    |
    v
Cache Lookup
  /     \
Hit     Miss
 |        |
 v        v
Restore  Run Command
           |
           v
        Cache Store
```

缓存实现包含：

* 64-bit FNV-1a 流式 Hash
* Cache Store / Restore
* 临时文件 + Rename 原子提交
* Metadata Size 校验
* Content Hash 校验
* Corruption Detection
* 缓存损坏时自动失效并回退正常构建

Cache 只影响性能，不影响构建正确性。

### Dirty Reason

使用：

```bash
./build/debug/forge --explain -j 3 build.forge
```

可以查看任务为什么进入本次 BuildPlan，例如：

```text
[dirty] a.cpp --compile--> a.o
  input newer than output: a.cpp -> a.o

[dirty] a.o + b.o + main.o --link--> app
  upstream output is dirty: a.o
```

目前可以解释包括：

```text
output missing
input newer than output
dynamic dependency changed
command changed
upstream output is dirty
```

这使增量构建行为不仅能够执行，也能够被诊断和验证。

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

主要模块：

| Module         | Responsibility                |
| -------------- | ----------------------------- |
| `BuildGraph`   | 管理 `Rule / Node / Edge` 及依赖关系 |
| `Builder`      | Dirty 判断与影响传播                 |
| `BuildPlanner` | 生成最小受影响 BuildPlan             |
| `Scheduler`    | 根据 DAG 依赖并行调度任务               |
| `Executor`     | 执行命令并维护日志、动态依赖与缓存             |
| `BuildLog`     | 保存历史构建信息与 Command Hash        |
| `DepsLog`      | 持久化动态 Header 依赖               |
| `LocalCache`   | 保存和恢复内容寻址构建结果                 |

---

## Quick Start

### Requirements

* Linux
* C++20 Compiler
* CMake
* GCC / Clang
* pthread

### Build

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

### Example Manifest

仓库根目录提供了一个简单 C++ 工程。

`build.forge`：

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

运行：

```bash
./build/debug/forge -j 3 build.forge
./app
```

程序输出：

```text
30
```

再次执行且输入没有变化时，不会重复构建 Clean Edge。

---

## Benchmark

Benchmark 工程包含：

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
   |
   v
1.062 s
```

约 **3.02× Speedup**，构建耗时下降约 **66.9%**。

并行效率不会随 Worker 数量无限线性增长，主要受到：

* 最终串行 Link
* Process Creation
* CPU / I/O 竞争
* Scheduler 同步开销

等因素影响。

### Cache Restore

删除全部 Output，但保留有效 Cache 后：

```text
-j4 Cold Build : 1.062 s
Cache Restore  : 0.086 s
```

10 次 Cache Restore：

```text
Average : 0.086 s
Median  : 0.080 s
```

相比 `-j4` Cold Build：

* **12.35× Speedup**
* 构建耗时下降约 **91.9%**

### Minimal Rebuild

修改单个源文件：

```text
benchmark/src/file_00.cpp
```

结果：

```text
planned edge count: 2
```

只执行：

```text
1 Compile
+
1 Link
```

其他 31 个 Compile Edge 不执行。

修改被 31 个模块共同依赖的 Header 后：

```text
planned edge count: 32
```

执行：

```text
31 Compile
+
1 Link
```

不依赖该 Header 的 Translation Unit 保持 Clean。

---

## Testing & Reliability

### CTest

运行：

```bash
ctest \
    --test-dir build/debug \
    --output-on-failure
```

当前共有 **12 项自动化测试**：

```text
scheduler_guard
scheduler_multi_output
build_planner_multi_output
scheduler_failure
hash
local_cache
compiler_identity
incremental_source
incremental_header
incremental_command
log_atomicity
interrupted_state
```

当前结果：

```text
12 / 12 tests passed
```

主要覆盖以下场景。

### Scheduler / Build Planner

* 非法调度状态保护
* Multi-Output Edge 去重
* Multi-Output BuildPlan 正确性
* Failure Propagation
* DAG 依赖约束下的任务调度

### Hash / Cache

* Incremental Hash
* Cache Key
* Compiler Identity
* Cache Store / Restore
* Atomic Commit
* Corruption Detection

### Incremental Rebuild

* Source Change
* Dynamic Header Change
* Command Change
* Dirty Propagation
* 最小受影响 BuildPlan
* `--explain` Dirty Reason

三个增量测试均在独立临时目录中执行真实 Compile / Link 流程，避免依赖开发目录中的历史构建产物。

### Persistent State Reliability

针对构建日志和持久化状态增加异常场景验证：

* `log_atomicity`：验证日志通过临时文件写入并原子替换正式文件，避免写入过程中断导致正式日志只保存部分内容
* `interrupted_state`：验证构建中断标记能够识别上一轮异常退出，避免下一次启动盲目信任可能不完整的构建状态

通过上述机制，使 ForgeBuild 在进程异常退出或日志更新被中断时能够采取更保守的恢复策略。

### Sanitizer

项目使用：

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

对自动化测试实际覆盖的执行路径进行内存安全与未定义行为检测。

最新代码在 ASan + UBSan 构建配置下执行完整测试集：

```text
12 / 12 tests passed
```

未检测到对应的 Sanitizer 报错。

此外，项目还使用 ThreadSanitizer 对并行调度场景进行过专项动态检测，并使用：

```text
4 Scheduler Workers
32 Compile Edges
1 Link Edge
```

进行并行构建场景验证。

> Sanitizer 属于动态分析，只能检查程序实际执行到的代码路径，因此该结果不等价于证明程序不存在任何 Bug。

### GitHub Actions

每次：

```text
push
or
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

CI 从干净 Runner 重新配置、构建并测试项目，避免本地历史构建产物、缓存或临时文件造成假成功。

---

## Design Decisions

### Incremental Build and Cache Are Different

**Incremental Build**

```text
Input unchanged
+
Output valid
      |
      v
Skip Edge
```

目标是避免执行本来就不需要执行的任务。

**Content Cache**

```text
Task needs execution
      |
      v
Cache Hit
      |
      v
Restore Result
```

目标是复用历史上已经计算过的结果。

两者解决的是不同问题。

### Dependency-Aware Parallelism

ForgeBuild 不会简单地把所有任务扔进线程池。

只有当一个 Edge 的所有必要前驱已经完成后，它才会进入 Ready Queue。

因此并行执行仍然满足 DAG 的依赖约束。

### Cache Correctness First

缓存是优化层：

```text
Cache Hit
    |
    v
Restore

Cache Miss / Corruption
    |
    v
Normal Build
```

即使缓存不存在、失效或损坏，也必须能够回退到正常构建路径。

### Dynamic Dependencies

Manifest 描述的是静态构建关系，但 C/C++ Header 依赖往往只有编译器真正运行后才能完整获得。

因此 ForgeBuild 将：

```text
Static Build Graph
+
Compiler Depfile
+
Persistent DepsLog
```

结合起来进行下一轮 Dirty Analysis。

---

## Project Structure

```text
ForgeBuild/
├── .github/workflows/    # GitHub Actions CI
├── benchmark/            # 32 Translation Unit benchmark
├── examples/             # Example projects
├── include/forge/        # Public headers
├── src/                  # Core implementation
├── tests/                # Automated tests
├── CMakeLists.txt
└── README.md
```

核心实现位于：

```text
include/forge/
src/
```

性能测试和自动化验证分别位于：

```text
benchmark/
tests/
```

---

## Current Scope

ForgeBuild 当前定位为 **单机 Linux 增量并行构建引擎**，重点验证构建图分析、增量重建、并行任务调度、动态依赖和本地内容寻址缓存等核心机制。

当前暂不实现：

* Windows Support
* Remote / Distributed Build
* Remote Cache
* Work Stealing
* File Watching
* Sandbox Execution
* 完整 Shell 语义
* CMake Project Generation
* 完整 Ninja Manifest 兼容

这些功能不属于当前项目的核心目标。

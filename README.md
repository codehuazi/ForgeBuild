# ForgeBuild

[![CI](https://github.com/codehuazi/ForgeBuild/actions/workflows/ci.yml/badge.svg)](https://github.com/codehuazi/ForgeBuild/actions/workflows/ci.yml)

**ForgeBuild** 是一个基于 **C++20 / Linux** 实现的增量并行构建引擎。

项目围绕 C/C++ 构建系统中的核心问题展开，实现了依赖 DAG、精确增量重建、动态 Header 依赖、Target 构建、并行任务调度、Linux 子进程执行、内容寻址本地缓存，以及持久化状态恢复和进程级构建目录互斥，并通过 CTest、Sanitizer、Benchmark 和 GitHub Actions 进行验证。

## Highlights

| Item | Result |
| --- | --- |
| Language / Platform | C++20 / Linux |
| Build Graph | Manifest Parser + DAG Validation |
| Incremental Build | Dirty Analysis + `--explain` |
| Target Build | Upstream Dependency Closure |
| Parallel Build | `-j N` Dependency-Aware Scheduler |
| Process Execution | `posix_spawn` + `waitpid` |
| Dynamic Dependencies | GCC / Clang Depfile + `DepsLog` |
| Local Cache | Content-Addressed Cache |
| Reliability | Atomic State + Recovery + Directory Lock |
| Regression Tests | **23 / 23 passed** |
| Parallel Speedup | `-j4` vs `-j1`: **3.02×** |
| Cache Restore | vs `-j4` Cold Build: **12.35×** |
| CI | GitHub Actions |

在 4 vCPU Ubuntu VM 的 32 Translation Unit Benchmark 中：

```text
-j1 Cold Build : 3.206 s
-j2 Cold Build : 1.648 s
-j4 Cold Build : 1.062 s
Cache Restore  : 0.086 s
```

- `-j4` 相比 `-j1`：约 **3.02×** 加速
- Cache Restore 相比 `-j4` Cold Build：约 **12.35×** 加速

---

## Quick Start

### Requirements

- Linux
- C++20 Compiler
- CMake >= 3.20
- GCC / Clang

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

### Manifest Example

ForgeBuild 使用自定义 Manifest 描述构建规则：

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

对应依赖关系：

```text
a.cpp ----> a.o ---\
                    \
b.cpp ----> b.o -----+--> app
                    /
main.cpp -> main.o -/
```

完整构建：

```bash
./build/debug/forge \
    -j 3 \
    build.forge
```

只构建指定 Target：

```bash
./build/debug/forge \
    -j 3 \
    build.forge \
    app
```

同时指定多个 Target：

```bash
./build/debug/forge \
    -j 3 \
    build.forge \
    target_a \
    target_b
```

查看重新构建原因：

```bash
./build/debug/forge \
    --explain \
    -j 3 \
    build.forge \
    app
```

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
                    BuildGraphValidator
                              |
                              v
                         Valid DAG
                              |
                 +------------+------------+
                 |                         |
            Full Build                Target(s)
                 |                         |
                 |                         v
                 |                 Upstream Dependency
                 |                      Closure
                 |                         |
                 +------------+------------+
                              |
                              v
                        Dirty Analysis
                              |
                              v
                         BuildPlanner
                              |
                              v
                      Minimal BuildPlan
                              |
                              v
                    Parallel Scheduler
                              |
                   +----------+----------+
                   |          |          |
                   v          v          v
                Worker     Worker     Worker
                   |          |          |
                   +----------+----------+
                              |
                              v
                           Executor
                              |
                  +-----------+-----------+
                  |                       |
                  v                       v
             Local Cache              ProcessRunner
                  |                       |
             Hit / Miss              posix_spawn
                  |                       |
                  |                    waitpid
                  |                       |
                  +-----------+-----------+
                              |
                              v
                       Depfile Parsing
                              |
                              v
                           DepsLog
                              |
                              v
                           BuildLog
```

构建过程同时受到：

```text
BuildDirectoryLock
+
Persisted-State Recovery
```

保护。

### Core Modules

| Module | Responsibility |
| --- | --- |
| `Parser` | 解析 Manifest 并诊断非法输入 |
| `BuildGraph` | 管理 Rule / Node / Edge 及依赖关系 |
| `BuildGraphValidator` | 在规划前验证 DAG 合法性 |
| `Builder` | Dirty Analysis、Dirty Propagation、Target Closure |
| `BuildPlanner` | 将 Dirty Edge 转换为有序 BuildPlan |
| `Scheduler` | 根据 DAG 依赖并行调度任务 |
| `Executor` | Cache、命令执行、Depfile 与状态更新 |
| `ProcessRunner` | Linux 子进程创建、等待和状态解析 |
| `BuildLog` | 保存历史 Command Hash |
| `DepsLog` | 保存动态 Header 依赖 |
| `LocalCache` | 保存和恢复内容寻址构建结果 |
| `BuildDirectoryLock` | 防止多个构建进程竞争同一目录 |

---

## Core Design

### Build Graph, Incremental Analysis and Targets

Manifest 首先被解析为：

```text
Rule
Node
Edge
BuildGraph
```

随后通过独立的 `BuildGraphValidator` 检查循环依赖等图语义错误：

```text
build.forge
    |
    v
Parser
    |
    v
BuildGraph
    |
    v
BuildGraphValidator
    |
    v
Valid DAG
```

因此 Parser 负责输入语法和结构，Scheduler 默认处理已经验证过的合法 DAG。

Builder 根据文件和历史状态判断 Edge 是否需要重新执行，主要考虑：

```text
Output Missing
Input Missing
Input Newer Than Output
Command Changed
Dynamic Dependency Changed
Upstream Edge Dirty
```

例如：

```text
a.cpp ----> a.o ---\
                    \
b.cpp ----> b.o -----+--> app
                    /
main.cpp -> main.o -/
```

如果只修改 `a.cpp`：

```text
a.cpp changed
     |
     v
a.o dirty
     |
     v
app dirty
```

本轮只重新编译 `a.cpp` 并重新链接 `app`。

使用：

```bash
./build/debug/forge \
    --explain \
    build.forge
```

可以查看 Edge 进入 BuildPlan 的具体原因。

ForgeBuild 同时支持指定 Target：

```bash
./build/debug/forge \
    build.forge \
    app
```

此时 Builder 不先分析整张图，而是从 Target 反向寻找 Producer：

```text
Target Node
     |
     v
Producer Edge
     |
     v
Input Nodes
     |
     v
Upstream Producers
     |
     v
Dependency Closure
```

然后仅在依赖闭包内执行 Dirty Analysis：

```text
Target
   |
   v
Dependency Closure
   |
   v
Dirty Analysis
   |
   v
Minimal BuildPlan
```

因此与当前 Target 无关的 Dirty 分支不会进入本轮构建。

同一张 DAG 上存在两个方向的遍历：

```text
Target Selection
Consumer -> Producer
向上游寻找必要依赖

Dirty Propagation
Producer -> Consumer
向下游传播失效状态
```

---

### Dynamic Header Dependencies

C/C++ Header 依赖无法完全依赖静态 Manifest 描述。

Compile Rule 使用：

```text
-MMD -MF $out.d
```

生成 GCC / Clang depfile：

```text
main.o: main.cpp a.hpp b.hpp
```

编译成功后，ForgeBuild 解析 depfile，并通过 `DepsLog` 持久化动态依赖：

```text
Static BuildGraph
       +
Compiler Depfile
       +
Persistent DepsLog
       |
       v
Dirty Analysis
```

因此修改 Header 时，只会重新执行真正受影响的 Translation Unit。

例如：

```text
a.hpp changed
     |
     +------> a.o
     |
     +------> main.o
                  |
                  v
                 app
```

---

### Parallel Scheduling and Linux Process Execution

BuildPlanner 得到最小 BuildPlan 后，由 Scheduler 根据 DAG 依赖执行并行调度：

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

Scheduler 根据未完成前驱依赖计数决定 Edge 是否可以进入 Ready Queue。

因此 `-j N` 并不是简单地把所有命令同时交给线程池，而是在保持 DAG 依赖约束的前提下并行执行。

调度层还处理：

```text
Failure Propagation
Multi-Output Edge
Worker Coordination
Ready Queue Synchronization
```

实际命令执行进一步从 Executor 中拆分为独立 `ProcessRunner`：

```text
Scheduler
    |
    v
Executor
    |
    v
ProcessRunner
    |
    +--> posix_spawn()
    |
    +--> waitpid()
    |
    v
/bin/sh -c <command>
```

通过 `/bin/sh -c` 保留 Manifest 中现有 Shell 命令语义。

ProcessRunner 负责：

```text
Process Spawn
waitpid
EINTR Retry
Normal Exit
Exit Code
Signal Termination
Spawn Failure
```

从而将构建语义与 Linux 子进程生命周期管理解耦。

---

### Content-Addressed Local Cache

当一个 Edge 确实需要执行时，Executor 会计算内容寻址 Cache Key：

```text
Cache Format Version
        +
Compiler Identity
        +
Expanded Command
        +
Dependency Paths
        +
Dependency Contents
        |
        v
     Cache Key
```

执行流程：

```text
Need Build
    |
    v
Cache Lookup
   /      \
 Hit      Miss
  |         |
  v         v
Restore   Run Command
             |
             v
          Cache Store
```

Cache Key 不只依赖时间戳，而是包含命令、编译器身份和输入内容，因此：

```text
Source Changed
Header Changed
Compiler Changed
Command Changed
```

都会产生不同的 Cache Key。

缓存对象采用临时文件和 Rename 提交，并进行 Metadata / Content 校验。

如果 Cache：

```text
Missing
Invalid
Corrupted
```

则直接回退到正常构建：

```text
Cache Failure
     |
     v
Normal Build
```

因此 Cache 是性能优化层，而不是构建正确性的必要条件。

---

### Persistent State and Build Directory Lock

ForgeBuild 使用：

```text
.forge_log
.forge_deps
```

保存增量构建需要的历史状态。

为了避免异常退出留下半写状态，日志通过临时文件和原子 Rename 提交。

构建开始时还会创建：

```text
.forge_in_progress
```

正常完成后：

```text
Build Start
    |
    v
Create Marker
    |
    v
Execute Build
    |
    v
Save DepsLog
    |
    v
Save BuildLog
    |
    v
Remove Marker
```

如果下一次启动仍发现 Marker：

```text
Previous Build Interrupted
          |
          v
Do Not Trust Old State
          |
          v
Conservative Rebuild
```

日志缺失、格式错误、损坏或状态不一致时，同样采用保守恢复策略。

这里遵循的原则是：

> 持久化日志是可以重新生成的辅助状态，不能为了复用旧状态而牺牲构建正确性。

线程同步之外，ForgeBuild 还需要处理两个独立进程同时构建同一目录的问题：

```text
Terminal A                  Terminal B
    |                           |
 ForgeBuild                  ForgeBuild
    |                           |
    +-------- same dir --------+
```

Scheduler 中的 `mutex` 无法解决这种进程间竞争，因此使用 `BuildDirectoryLock`：

```text
Process A
   |
   +--> acquire .forge_lock
   |
   v
 Build

Process B
   |
   +--> lock already held
   |
   v
 Fail Fast
```

底层采用：

```text
open(O_CLOEXEC)
+
flock(LOCK_EX | LOCK_NB)
```

并通过 RAII 管理文件描述符和 Lock 生命周期。

这样可以防止多个 ForgeBuild 实例同时修改：

```text
Build Outputs
.forge_log
.forge_deps
.forge_cache
```

至此，ForgeBuild 的可靠性链路覆盖：

```text
Input Validation
       |
       v
Graph Validation
       |
       v
Incremental State Validation
       |
       v
Process Execution
       |
       v
Atomic Persistent State
       |
       v
Cross-Process Directory Lock
```

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

### Parallel Build

Cold Build 每种并行度执行 5 次：

| Workers | Average | Speedup | Parallel Efficiency |
| --- | ---: | ---: | ---: |
| `-j1` | 3.206 s | 1.00× | 100% |
| `-j2` | 1.648 s | 1.95× | 97.3% |
| `-j4` | 1.062 s | 3.02× | 75.5% |

因此：

```text
-j1 : 3.206 s
-j4 : 1.062 s
```

`-j4` 相比 `-j1`：

- **3.02× Speedup**
- 构建耗时下降约 **66.9%**

并行度继续增加时不会保持完全线性加速，主要受最终 Link、进程创建、CPU / I/O 竞争以及调度同步开销影响。

### Cache Restore

删除构建 Output、保留有效 Cache 后：

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

- **12.35× Speedup**
- 构建耗时下降约 **91.9%**

### Minimal Rebuild

修改单个 Source：

```text
benchmark/src/file_00.cpp
```

结果：

```text
planned edge count: 2
```

仅执行：

```text
1 Compile
+
1 Link
```

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

未依赖该 Header 的 Translation Unit 保持 Clean。

---

## Testing & Reliability

### CTest

运行：

```bash
ctest \
    --test-dir build/debug \
    --output-on-failure
```

当前共有 **23 项自动化回归测试**：

```text
 1  scheduler_guard
 2  scheduler_multi_output
 3  build_planner_multi_output
 4  scheduler_failure
 5  hash
 6  local_cache
 7  compiler_identity
 8  incremental_source
 9  incremental_header
10  incremental_command
11  incremental_missing_input
12  missing_output
13  log_atomicity
14  persisted_state_format
15  corrupted_persisted_state
16  interrupted_state
17  manifest_multi_output
18  cycle_detection
19  parser_error
20  process_runner
21  build_directory_lock
22  concurrent_build_lock
23  target_selection
```

当前 Debug：

```text
23 / 23 tests passed
```

覆盖范围包括：

```text
DAG / Scheduler
Multi-Output
Failure Propagation
Incremental Source / Header / Command
Missing Input / Output
Dirty Propagation
Target Selection
Hash / Compiler Identity
Local Cache
Persisted-State Recovery
Manifest Validation
Cycle Detection
ProcessRunner
Build Directory Lock
```

---

### Sanitizer

项目维护独立：

```text
build/sanitize
```

配置：

```text
AddressSanitizer
+
UndefinedBehaviorSanitizer
```

完整执行：

```bash
ctest \
    --test-dir build/sanitize \
    --output-on-failure
```

当前结果：

```text
23 / 23 tests passed
```

在自动化测试实际覆盖到的执行路径中，没有检测到对应 ASan / UBSan 错误。

> Sanitizer 属于动态分析，该结果不等价于证明程序不存在任何 Bug。

---

### GitHub Actions

CI 在 Push / Pull Request 时自动执行：

```text
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

使用干净 Ubuntu Runner 重新配置、构建并测试项目，避免本地 Build Artifact 或 Cache 造成假成功。

---


## Project Structure

```text
ForgeBuild/
├── .github/
│   └── workflows/        # GitHub Actions CI
├── benchmark/            # Benchmark generator / sources
├── examples/             # Example projects
├── include/forge/        # Core headers
├── src/                  # Core implementation
├── tests/                # Automated tests
├── CMakeLists.txt
├── build.forge
└── README.md
```

---

## Current Scope

ForgeBuild 当前定位为：

> **单机 Linux C/C++ 增量并行构建引擎**

重点覆盖：

```text
Build Graph
DAG Validation
Incremental Rebuild
Target Build
Dynamic Dependencies
Parallel Scheduling
Linux Process Execution
Local Content Cache
Persistent-State Reliability
Inter-Process Build Lock
```

当前不实现：

```text
Windows Support
Remote / Distributed Build
Remote Cache
Work Stealing
File Watching
Sandbox Execution
CMake Project Generation
Full Ninja Manifest Compatibility
```

这些能力不属于当前项目的核心目标。
ForgeBuild Day7 学习笔记
文件状态检测与时间戳增量构建
一、Day7 学习目标

Day7 的核心目标，是让 ForgeBuild 从“每次都依赖人工标记 dirty”升级为“根据真实磁盘文件状态决定是否构建”。

最终实现的构建链：

main.cpp
    │
    │ compile
    ▼
main.o
    │
    │ link
    ▼
app

系统能够正确处理以下四种情况：

场景	构建计划
main.o、app 都不存在	编译 + 链接
所有文件都没有变化	不执行任何命令
main.cpp 被更新	编译 + 链接
只删除 app	只链接

Day7 最终完成了：

磁盘文件状态读取
        +
时间戳过期判断
        +
输出缺失判断
        +
下游依赖传播
        +
拓扑排序
        +
真实命令执行
二、为什么需要真实文件状态

Day4 阶段，我们曾经通过：

cpp->mark_dirty();

人为模拟源文件发生变化。

这种方式适合学习 dirty 传播，但不能用于真正的增量构建。

真实构建系统应该根据磁盘状态判断：

源文件比输出文件新
    → 重新构建

输出文件不存在
    → 重新构建

输出文件存在且不旧
    → 跳过构建

例如：

main.cpp 修改时间：10:00
main.o   修改时间：09:50

因为：

main.cpp > main.o

说明 main.o 已经过期，需要重新编译。

三、FileSystem 模块

Day7 新增了一个文件系统工具类，用于读取文件是否存在和文件修改时间。

1. 头文件

文件：

include/forge/file_system.hpp

内容：

#pragma once

#include <string>

namespace forge
{

class FileSystem
{
public:
    static bool exists(
        const std::string& path
    );

    static long long timestamp(
        const std::string& path
    );
};

}
2. 实现文件

文件：

src/file_system.cpp

内容：

#include "forge/file_system.hpp"

#include <filesystem>

namespace forge
{

bool FileSystem::exists(
    const std::string& path
)
{
    return std::filesystem::exists(path);
}


long long FileSystem::timestamp(
    const std::string& path
)
{
    if (!exists(path))
    {
        return 0;
    }

    auto time =
        std::filesystem::last_write_time(path);

    return time
        .time_since_epoch()
        .count();
}

}
3. 为什么不存在的文件返回 0

代码：

if (!exists(path))
{
    return 0;
}

这里用 0 表示文件不存在。

不过判断文件是否存在时，不能只依赖时间戳，因为真实文件时间戳可能是负数。

正确逻辑是：

if (!output->exists())
{
    return true;
}

先判断存在性，再比较时间戳。

4. 为什么时间戳可能是负数

测试中出现过：

main.cpp timestamp: -4652844804157874421
main.o timestamp:   -4652843603074550070

这是正常现象。

std::filesystem::file_time_type 使用的 epoch 由标准库实现决定，不保证与 Unix 时间戳使用相同起点。

增量构建不需要把时间戳转换成人类日期，只需要比较大小：

input->timestamp() > output->timestamp()

只要两个时间戳来自同一种时钟，比较结果就是有效的。

四、Node 增加磁盘状态缓存

Node 原本只保存：

path
exists
dirty

Day7 增加：

timestamp
1. Node 新增成员
long long timestamp_ = 0;
2. Node 新增接口
long long timestamp() const;

void refresh();
3. Node::refresh() 实现

文件：

src/node.cpp

加入：

#include "forge/file_system.hpp"

实现：

long long Node::timestamp() const
{
    return timestamp_;
}


void Node::refresh()
{
    exists_ =
        FileSystem::exists(path_);

    timestamp_ =
        FileSystem::timestamp(path_);
}
4. refresh() 的作用

Node 是构建图中的内存对象，而文件存在于磁盘。

每次程序启动后，Node 默认状态并不知道磁盘情况，例如：

exists_ = false;
timestamp_ = 0;

但磁盘上可能已经有：

main.o
app

因此在生成构建计划前，必须调用：

node->refresh();

把磁盘状态同步到 Node：

磁盘文件状态
    ↓
Node::exists_
Node::timestamp_
五、FileSystem 和 Node 测试

测试文件：

tests/file_system_demo.cpp

测试内容包括：

main.cpp 存在
main.o 存在
missing.file 不存在

并验证：

source_node.exists()
    ==
FileSystem::exists("main.cpp")

以及：

source_node.timestamp()
    ==
FileSystem::timestamp("main.cpp")

运行结果：

main.cpp exists: true
main.o exists: true
missing.file exists: false

source node exists: true
output node exists: true
missing node exists: false

file system checks passed

这证明：

FileSystem 读取正确
Node::refresh() 同步正确
六、Edge 的增量构建判断

Day7 重新实现：

bool Edge::needs_build() const

核心代码：

bool Edge::needs_build() const
{
    for (const auto* output : outputs_)
    {
        if (!output->exists())
        {
            return true;
        }
    }

    for (const auto* input : inputs_)
    {
        for (const auto* output : outputs_)
        {
            if (
                input->timestamp()
                >
                output->timestamp()
            )
            {
                return true;
            }
        }
    }

    return false;
}
1. 规则一：输出不存在
for (const auto* output : outputs_)
{
    if (!output->exists())
    {
        return true;
    }
}

例如：

main.cpp 存在
main.o 不存在

必须执行：

g++ -c main.cpp -o main.o
2. 规则二：输入比输出新
if (
    input->timestamp()
    >
    output->timestamp()
)
{
    return true;
}

例如：

main.cpp = 300
main.o   = 200

因为：

300 > 200

所以 main.o 已经过期。

3. 为什么使用双层循环

一条 Edge 可能有多个输入和多个输出：

输入：
main.cpp
common.hpp

输出：
main.o
main.d

只要任意输入比任意输出新，就可能需要重新构建。

因此使用：

for (input)
{
    for (output)
    {
        compare timestamp
    }
}
七、Builder 刷新所有 Node

在生成构建计划之前，Builder 必须先刷新整张图。

新增：

void Builder::refresh_nodes()
{
    for (
        const auto& entry :
        graph_.nodes()
    )
    {
        entry.second->refresh();
    }
}

这里：

entry.first

是节点路径。

entry.second

是：

std::unique_ptr<Node>

所以：

entry.second->refresh();

会刷新每个 Node。

八、单条 Edge 的增量验证

最初测试构建链：

main.cpp → main.o

在 executor_demo 中删除了：

cpp->mark_dirty();

之后完全依赖真实磁盘状态。

1. 输出已存在且较新

运行：

./build/debug/executor_demo

结果：

plan edge count: 0
===== Execute Build =====
execute result: true

说明没有重复编译。

2. 更新源文件

执行：

touch main.cpp
./build/debug/executor_demo

结果：

plan edge count: 1
g++ -c main.cpp -o main.o

说明源文件变新后会重新编译。

3. 再次运行
./build/debug/executor_demo

结果：

plan edge count: 0

因为刚生成的 main.o 已经是最新的。

4. 删除输出

执行：

rm main.o
./build/debug/executor_demo

结果：

plan edge count: 1
g++ -c main.cpp -o main.o

说明输出丢失会触发重建。

九、时间戳判断的多级依赖问题

单条 Edge 使用时间戳判断没有问题。

但多级依赖存在一个重要问题：

main.cpp
    ↓
main.o
    ↓
app

假设磁盘状态为：

main.cpp = 300
main.o   = 200
app      = 250

检查 compile Edge：

main.cpp 300 > main.o 200

所以需要编译。

但是检查 link Edge：

main.o 200 < app 250

单看当前磁盘状态，会认为不需要链接。

这是错误的。

因为 compile 执行后：

main.o 会被重新生成

它的新时间可能变成：

main.o = 301

这时：

main.o 301 > app 250

所以 app 也必须重新链接。

结论：

增量构建不仅要看当前磁盘状态，还要考虑某个输入是否会在本轮构建中被重新生成。

十、重新定义 dirty 的含义

Day4 的 dirty 是人为设置的：

cpp->mark_dirty();

Day7 中，dirty 更准确的含义变成：

这个 Node 会在本轮构建中被重新生成

例如：

compile Edge 需要执行
    ↓
main.o 会变化
    ↓
main.o.mark_dirty()
    ↓
使用 main.o 的 link Edge 也需要执行

因此 dirty 不再是构建判断的起点，而是下游传播的中间状态。

十一、收集需要构建的 Edge

新增函数：

std::vector<Edge*>
Builder::collect_edges_to_build()

实现：

std::vector<Edge*>
Builder::collect_edges_to_build()
{
    std::vector<Edge*> result;

    std::queue<Edge*> pending_edges;

    std::unordered_set<Edge*> queued_edges;


    // 第一阶段：
    // 找到因真实磁盘状态直接过期的 Edge。
    for (
        const auto& edge_owner :
        graph_.edges()
    )
    {
        Edge* edge =
            edge_owner.get();

        if (!edge->needs_build())
        {
            continue;
        }

        pending_edges.push(edge);

        queued_edges.insert(edge);
    }


    // 第二阶段：
    // 向下游传播。
    while (!pending_edges.empty())
    {
        Edge* edge =
            pending_edges.front();

        pending_edges.pop();

        result.push_back(edge);


        for (Node* output : edge->outputs())
        {
            output->mark_dirty();


            for (
                Edge* dependent_edge :
                output->out_edges()
            )
            {
                bool inserted =
                    queued_edges
                        .insert(dependent_edge)
                        .second;

                if (inserted)
                {
                    pending_edges.push(
                        dependent_edge
                    );
                }
            }
        }
    }


    return result;
}
十二、该算法的两个阶段
第一阶段：找直接过期的 Edge

代码：

if (edge->needs_build())
{
    pending_edges.push(edge);
    queued_edges.insert(edge);
}

直接过期的原因包括：

输出不存在
输入比输出新

例如：

main.cpp > main.o

则 compile Edge 进入队列。

第二阶段：向下游传播

取出一条 Edge：

Edge* edge =
    pending_edges.front();

pending_edges.pop();

然后把它加入最终结果：

result.push_back(edge);

遍历输出：

for (Node* output : edge->outputs())

将输出标记为 dirty：

output->mark_dirty();

然后通过：

output->out_edges()

找到所有使用这个输出的下游 Edge。

例如：

compile 输出 main.o

main.o.out_edges()
    ↓
link Edge

于是 link Edge 被加入队列。

十三、为什么需要 unordered_set

构建图可能存在多个输入汇聚到同一条 Edge：

a.o ─┐
     ├→ link
b.o ─┘

如果 a.o 和 b.o 都变化，link Edge 可能被发现两次。

所以使用：

std::unordered_set<Edge*> queued_edges;

判断：

bool inserted =
    queued_edges
        .insert(dependent_edge)
        .second;

其中：

inserted == true

表示第一次加入。

inserted == false

表示之前已经加入，不再重复入队。

十四、Builder 最终流程

原来的 Builder::build() 是：

从人工 dirty Node 开始传播
    ↓
收集 dirty Edge

Day7 改成：

BuildPlan Builder::build()
{
    refresh_nodes();

    auto edges =
        collect_edges_to_build();

    BuildPlanner planner;

    auto ordered =
        planner.plan(edges);

    BuildPlan plan;

    for (auto* edge : ordered)
    {
        plan.add_edge(edge);
    }

    return plan;
}
各部分职责
refresh_nodes()

负责：

同步真实磁盘状态
collect_edges_to_build()

负责：

选择直接过期的任务
+
向下游传播
BuildPlanner::plan()

负责：

拓扑排序
BuildPlan

负责：

保存最终有序任务列表
十五、修复的一个重要 Bug

原来的代码中：

auto ordered =
    planner.plan(edges);

已经得到了拓扑排序结果。

但后面却写成：

for (auto* edge : edges)
{
    plan.add_edge(edge);
}

这意味着排序结果 ordered 完全没有被使用。

正确代码：

for (auto* edge : ordered)
{
    plan.add_edge(edge);
}

否则即使 BuildPlanner 排出了：

compile → link

最终执行的仍可能是原始顺序：

link → compile
十六、两级增量测试

新增测试：

tests/incremental_chain_demo.cpp

构建链：

main.cpp → main.o → app

为了验证构建顺序不依赖 Edge 创建顺序，测试中故意先创建 link Edge：

auto* link_edge =
    graph.create_edge(link_rule);

后创建 compile Edge：

auto* compile_edge =
    graph.create_edge(compile_rule);

原始容器顺序可能是：

link
compile

但最终计划必须是：

compile
link
十七、多级测试的四种结果
场景一：两个输出都不存在

执行：

rm -f main.o app
./build/debug/incremental_chain_demo

结果：

plan edge count: 2
planned commands:
  g++ -c main.cpp -o main.o
  g++ main.o -o app

说明：

compile 和 link 都进入计划
拓扑顺序正确

运行：

./app

输出：

Hello ForgeBuild
场景二：没有任何变化

再次执行：

./build/debug/incremental_chain_demo

结果：

plan edge count: 0
planned commands:
===== Execute Build =====
execute result: true

说明：

compile 被跳过
link 被跳过
场景三：更新 main.cpp

执行：

touch main.cpp
./build/debug/incremental_chain_demo

结果：

plan edge count: 2
planned commands:
  g++ -c main.cpp -o main.o
  g++ main.o -o app

传播过程：

main.cpp > main.o
    ↓
compile Edge 直接过期
    ↓
main.o 标记 dirty
    ↓
找到 main.o.out_edges()
    ↓
link Edge 加入计划

这证明：

时间戳判断
+
下游传播

组合成功。

场景四：只删除 app

执行：

rm app
./build/debug/incremental_chain_demo

结果：

plan edge count: 1
planned commands:
  g++ main.o -o app

说明：

main.o 已经最新
app 不存在
所以只执行 link
十八、清理旧的 dirty 传播代码

旧 Builder 中保留过：

void propagate_dirty(Node* node);

std::vector<Edge*>
collect_dirty_edges();

这两个函数属于 Day4 的旧流程。

通过：

grep -R \
"propagate_dirty\|collect_dirty_edges" \
-n include src tests

确认只有声明和定义，没有外部使用后，删除了：

Builder::propagate_dirty()
Builder::collect_dirty_edges()

再次搜索时无输出：

grep 没有匹配结果

重新编译：

[12/12] Linking CXX executable incremental_chain_demo

然后执行回归测试：

touch main.cpp
./build/debug/incremental_chain_demo

仍然得到：

plan edge count: 2
compile
link

说明删除的是废弃代码，新流程没有受到影响。

十九、Day7 最终代码流程图
Builder::build()
    │
    ▼
refresh_nodes()
    │
    │ 读取 exists / timestamp
    ▼
collect_edges_to_build()
    │
    ├─ output 不存在
    ├─ input 比 output 新
    └─ 向下游传播
    │
    ▼
BuildPlanner::plan()
    │
    │ 拓扑排序
    ▼
BuildPlan
    │
    ▼
Executor::execute()
    │
    ├─ compile
    └─ link
二十、Day7 核心知识点总结
1. 增量构建的基本判断
输出不存在
    → 构建

输入比输出新
    → 构建

输出存在且不旧
    → 跳过
2. Node 是磁盘文件的内存表示

Node 需要保存：

path
exists
timestamp
dirty

通过：

Node::refresh()

同步磁盘状态。

3. 当前时间戳不等于未来构建状态

在多级依赖中，当前磁盘上的中间文件可能很旧，但它会在本轮被重新生成。

所以不能只看：

main.o 当前是否比 app 新

还要考虑：

main.o 是否会在本轮发生变化
4. dirty 的新语义

Day7 中：

dirty Node
=
该 Node 会在当前构建计划中被重新生成

它用于向下游传播，不再依赖人工调用：

source->mark_dirty();
5. 选择任务和排序任务必须分离
collect_edges_to_build()
负责选哪些任务

BuildPlanner
负责按什么顺序执行

这是良好的职责划分。

6. 拓扑排序结果必须真正使用

错误：

auto ordered =
    planner.plan(edges);

for (auto* edge : edges)

正确：

auto ordered =
    planner.plan(edges);

for (auto* edge : ordered)
二十一、Day7 完成状态
FileSystem 文件存在性检测           ✅
FileSystem 文件时间戳读取           ✅
Node::refresh()                     ✅
输出缺失触发构建                    ✅
输入更新触发构建                    ✅
无变化跳过构建                      ✅
多级依赖传播                        ✅
拓扑排序                            ✅
真实编译和链接                      ✅
旧 dirty 传播代码清理               ✅
Day7 最终结论

ForgeBuild 已经从：

人工标记 dirty 的构建模拟器

升级为：

能够读取真实磁盘状态、
判断输出是否过期、
传播多级依赖、
并执行增量编译与链接的构建引擎
ForgeBuild Day6 学习笔记：Command Executor（命令执行器）

日期：Day6
主题：

让 ForgeBuild 从“知道需要构建什么”升级到“真正执行构建命令”。

一、Day6目标

Day5结束后，ForgeBuild已经完成：

Parser
    ↓
Manifest
    ↓
BuildGraph
    ↓
Builder
    ↓
BuildPlanner
    ↓
BuildPlan

此时系统只能：

解析构建描述
建立依赖图
找到需要构建的 Edge
排出正确构建顺序

但是还不能真正编译。

例如：

BuildPlan知道：

main.cpp --compile--> main.o

但是不知道：

执行什么命令？

因此Day6实现：

BuildPlan
    ↓
Executor
    ↓
Edge生成command
    ↓
执行shell命令
    ↓
生成目标文件
二、为什么需要 Executor？
错误设计

直接在 Builder 里面执行：

Builder::build()
{
    system(command);
}

问题：

Builder职责混乱。

Builder本应该负责：

找出需要构建的任务

而不是：

如何执行任务

因此拆分：

模块	职责
Builder	发现需要构建的Edge
BuildPlanner	决定执行顺序
BuildPlan	保存执行计划
Executor	执行命令

最终结构：

Builder

负责：
what to build


BuildPlanner

负责：
when to build


Executor

负责：
how to build

三、Day6新增模块

新增：

include/forge/executor.hpp

src/executor.cpp

Executor设计：

class Executor
{

public:

    bool execute(
        const BuildPlan& plan
    );

};

为什么返回 bool？

因为命令可能失败：

例如：

g++ main.cpp

可能：

file not found

所以需要反馈：

成功：

true

失败：

false
四、Edge增加命令生成能力
之前的Edge

Edge只能表示：

main.cpp --compile--> main.o

包含：

Rule* rule_;

inputs_;

outputs_;

但是执行命令需要：

g++ -c main.cpp -o main.o

所以增加：

std::string command() const;
五、为什么 command() 放 Edge？

因为 Edge 拥有完整信息。

Rule：

保存模板：

g++ -c $in -o $out

Edge：

知道：

输入：

main.cpp

输出：

main.o

所以：

Edge负责：

模板
+
上下文

↓

真实命令

Executor不需要知道：

Rule结构
输入节点
输出节点

它只需要：

edge->command()

得到：

g++ -c main.cpp -o main.o
六、实现字符串变量替换
遇到的问题

最开始：

replace(
    cmd,
    "$in",
    input
);

编译失败：

no matching function for call to replace

原因：

C++标准库中的：

std::replace()

不是字符串替换。

它用于：

字符替换：

例如：

a -> b

不是：

"$in" -> "main.cpp"

所以自己实现：

replace_all()

位置：

string_utils.hpp
string_utils.cpp

功能：

输入：

g++ -c $in -o $out

执行：

replace_all(
    cmd,
    "$in",
    "main.cpp"
);

得到：

g++ -c main.cpp -o $out

继续：

replace_all(
    cmd,
    "$out",
    "main.o"
);

得到：

g++ -c main.cpp -o main.o
七、Executor执行命令

Executor核心逻辑：

for(auto* edge : plan.edges())
{

    std::string cmd =
        edge->command();


    int result =
        std::system(
            cmd.c_str()
        );


    if(result != 0)
    {
        return false;
    }

}

执行流程：

例如：

BuildPlan：

[
 compile_main
]

Executor：

调用：

edge->command()

得到：

g++ -c main.cpp -o main.o

然后：

system()

执行。

八、添加 executor_demo

为了测试Executor：

新增：

tests/executor_demo.cpp

测试流程：

创建Manifest

↓

创建Rule

↓

创建Node

↓

创建Edge

↓

生成BuildPlan

↓

Executor执行


测试命令：

Rule:

"g++ -c $in -o $out"

输入：

main.cpp

输出：

main.o

最终执行：

g++ -c main.cpp -o main.o
九、测试文件 main.cpp

注意：

这里的main.cpp不是ForgeBuild源码。

它是：

被ForgeBuild构建的用户代码。

目录：

ForgeBuild

├── main.cpp     <-- 测试输入

├── src
├── include
├── tests
└── build


内容：

#include <iostream>

int main()
{
    std::cout
        << "Hello ForgeBuild\n";

    return 0;
}
十、遇到的问题总结
问题1：executor_demo不存在

错误：

./build/debug/executor_demo:
没有那个文件或目录

原因：

CMake没有生成该目标。

解决：

CMakeLists增加：

add_executable(
    executor_demo
    tests/executor_demo.cpp
)

target_link_libraries(
    executor_demo
    forge_core
)

重新：

cmake -S . -B build/debug

cmake --build build/debug
问题2：Edge incomplete type

错误：

invalid use of incomplete type 'class forge::Edge'

原因：

只有：

class Edge;

前向声明。

只能：

Edge*

不能：

edge->command()

解决：

executor.cpp增加：

#include "forge/edge.hpp"
C++规则总结
只保存指针：

可以：

class Edge;

例如：

Edge* edge;
调用成员函数：

必须：

#include "edge.hpp"

例如：

edge->command();
问题3：execute result:1误判

输出：

execute result: 1

以为失败。

实际：

因为：

bool

默认输出：

true:

1

false:

0

改：

std::boolalpha

输出：

execute result: true
问题4：执行成功但是状态不同步

执行：

g++ -c main.cpp -o main.o

磁盘：

main.o存在

但是Node：

exists_=false

原因：

文件系统状态和内存状态没有同步。

解决：

增加：

Node::mark_exists()

执行成功后：

for(auto* output : edge->outputs())
{
    output->mark_exists();

    output->mark_clean();
}
十一、Day6最终代码架构

现在：

ForgeBuild

include
|
├── parser.hpp
├── manifest.hpp
├── build_graph.hpp
├── node.hpp
├── edge.hpp
├── builder.hpp
├── build_planner.hpp
├── build_plan.hpp
└── executor.hpp


src
|
├── parser.cpp
├── manifest.cpp
├── build_graph.cpp
├── node.cpp
├── edge.cpp
├── builder.cpp
├── build_planner.cpp
├── build_plan.cpp
└── executor.cpp

十二、Day6完成状态
功能	完成
Edge生成command	✅
$in替换	✅
$out替换	✅
replace_all工具	✅
Executor模块	✅
调用system执行命令	✅
真实调用gcc	✅
生成main.o	✅
Node状态更新	✅
十三、Day6核心收获

今天最重要的不是写了多少代码，而是理解了构建系统的核心分层：

Graph

描述关系


Builder

发现任务


Planner

安排顺序


Executor

执行任务


Node

记录状态


这已经是 Ninja/CMake 类构建系统的核心思想。

十四、当前ForgeBuild能力

现在已经可以：

输入：

main.cpp

规则：

g++ -c $in -o $out

自动：

找到需要构建任务
生成命令
调用gcc
生成main.o
更新状态

下一阶段 Day7：

Incremental Build（增量构建）

目标：

实现：

第一次：

build main.o

第二次：

nothing to build

修改：

main.cpp

之后：

rebuild main.o
ForgeBuild Day5：BuildPlanner 与任务调度
今日目标

实现：

根据构建依赖关系，将需要执行的 Edge 排序，生成正确 BuildPlan。

1. 为什么需要 BuildPlanner？

之前：

Builder：

负责：

dirty传播

寻找需要构建的Edge

但是：

不知道：

哪个任务先执行

因此拆分：

Builder

负责：
任务发现


BuildPlanner

负责：
任务排序


BuildPlan

负责：
保存执行顺序

2. Edge依赖关系

定义：

Edge::depends_on()

判断：

当前Edge输入节点：

是否来自另一个Edge输出。

例如：

compile:

output:
main.o


link:

input:
main.o


所以：

link depends compile
3. 拓扑排序

使用：

Kahn Algorithm。

步骤：

初始化入度
compile = 0

link = 1
入度0加入队列
queue:

compile
移除任务

执行：

compile

更新：

link:

1 -> 0
得到结果
compile

link

4. 循环依赖检测

如果：

A -> B
B -> A

没有入度0节点。

判断：

result.size()
!=
edges.size()

说明：

存在环。

5. 今日遇到的问题
问题1：
incomplete type

错误：

invalid use of incomplete type Edge

原因：

只包含：

class Edge;

只能声明指针。

调用：

edge->function()

需要：

#include "forge/edge.hpp"
问题2：
为什么BuildPlan数量从1变2？

Day4：

只有：

compile

Day5：

增加：

compile

link

所以：

Plan edges:

1 -> 2

正常。

问题3：
为什么需要拓扑排序？

因为：

Edge创建顺序 ≠ 构建顺序。

构建顺序由：

依赖关系

决定。

七、你的 ForgeBuild 当前架构

现在已经达到：

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


对应 Ninja：

ForgeBuild	Ninja
Node	Node
Edge	Edge
Rule	Rule
BuildPlanner	Plan / Graph traversal
BuildPlan	Build order
八、Day6 下一步

下一阶段非常关键：

现在你的 ForgeBuild：

已经知道：

需要执行什么

执行顺序是什么

但是：

还不能真正执行。

Day6：

Command Executor（命令执行器）

目标：

让：

compile Edge

真正执行：

g++ -c main.cpp -o main.o

会加入：

Executor

Rule command

变量替换:

$in

$out


最后：

ForgeBuild会从：

构建系统模拟器

进入：

真正可以编译项目的小型Ninja
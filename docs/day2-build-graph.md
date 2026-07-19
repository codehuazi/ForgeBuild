# Day2：Build Graph

## 1. 核心对象

### Rule

Rule 表示通用构建方式，例如：

```text
compile: g++ -c $in -o $out

Rule 不知道具体输入输出，因此不能独立执行。

Edge

Edge 表示一次具体构建实例，例如：

main.cpp -> main.o

Edge 保存：

使用的 Rule
输入 Node
输出 Node
Node

Node 表示构建图中的资源，例如：

main.cpp
main.o
app

Node 保存：

路径
唯一生产 Edge
所有消费 Edge
BuildGraph

BuildGraph 负责：

拥有所有 Node 和 Edge
保证同一路径对应同一个 Node
建立 Node 和 Edge 的双向关系
拒绝重复生产者
2. 所有权

BuildGraph 使用 unique_ptr 拥有 Node 和 Edge。

Node 和 Edge 之间使用普通指针互相引用，但不拥有对方。

BuildGraph
├── owns Node
└── owns Edge

Node <-> Edge
3. 前向声明

前向声明可以用于：

Node*
Node&

但不能用于：

创建 Node 对象
调用 Node 成员函数
sizeof(Node)

unique_ptr<Node> 可以和前向声明配合，但拥有者的析构函数应在看到 Node 完整定义的 .cpp 中定义。

4. 图不变量
Node 唯一性

同一路径必须对应同一个 Node。

单生产者

一个输出 Node 最多只有一个生产 Edge。

双向一致性

若 Edge 将 Node 作为输入：

Edge.inputs 包含 Node
Node.out_edges 包含 Edge

若 Edge 生成 Node：

Edge.outputs 包含 Node
Node.in_edge 指向 Edge
5. 当前 DAG
main.cpp --compile--> main.o --\
                                link --> app
math.cpp --compile--> math.o --/

包含：

5 个 Node
3 个 Edge
2 个 Rule

这份笔记后面会直接成为项目文档和面试复习材料。
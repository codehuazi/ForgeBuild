# ForgeBuild Day2 学习笔记：核心构建图模型

## 一、今日目标

Day2 的目标是用 C++ 建立 ForgeBuild 最核心的数据模型，并能够手动构造、验证一张完整的构建依赖图。

最终构造的 DAG：

```text
main.cpp ──compile──> main.o ──┐
                               ├──link──> app
math.cpp ──compile──> math.o ──┘
```

包含：

- 5 个 `Node`
- 3 个 `Edge`
- 2 个 `Rule`
- 1 个 `BuildGraph`

---

## 二、核心概念

### 1. Rule：构建规则模板

`Rule` 描述“如何构建”，例如：

```text
compile: g++ -c $in -o $out
link:    g++ $in -o $out
```

它只保存：

```cpp
std::string name_;
std::string command_;
```

`Rule` 不知道具体输入和输出，因此它只是可复用的命令模板。

两个不同的编译任务可以共享同一个 `compile` Rule：

```text
main.cpp -> main.o
math.cpp -> math.o
```

### 2. Node：构建图中的资源

`Node` 表示一个文件或构建产物，例如：

```text
main.cpp
main.o
app
```

核心数据：

```cpp
std::string path_;
Edge* in_edge_{nullptr};
std::vector<Edge*> out_edges_;
```

含义：

- `path_`：资源路径
- `in_edge_`：生成该 Node 的唯一 Edge
- `out_edges_`：所有依赖该 Node 的 Edge

重要规则：

- 源文件通常没有生产者，因此 `in_edge_ == nullptr`
- 一个 Node 最多只有一个生产者
- 一个 Node 可以被多个 Edge 消费

### 3. Edge：一次具体的构建动作

`Edge` 表示一次具体构建实例，例如：

```text
main.cpp --compile--> main.o
```

核心数据：

```cpp
Rule* rule_;
std::vector<Node*> inputs_;
std::vector<Node*> outputs_;
```

它保存：

- 使用哪个 Rule
- 输入 Node
- 输出 Node

`Rule` 与 `Edge` 的区别：

```text
Rule = 怎么构建
Edge = 对哪些输入输出执行这次构建
```

### 4. BuildGraph：图的拥有者和管理者

`BuildGraph` 负责：

- 拥有所有 Node
- 拥有所有 Edge
- 保证同一路径只对应一个 Node
- 建立 Node 与 Edge 的双向关系
- 拒绝重复生产者

核心容器：

```cpp
std::unordered_map<
    std::string,
    std::unique_ptr<Node>
> nodes_;

std::vector<
    std::unique_ptr<Edge>
> edges_;
```

---

## 三、所有权设计

### 1. BuildGraph 拥有对象

```text
BuildGraph
├── owns Node
└── owns Edge
```

使用 `std::unique_ptr` 表达唯一所有权：

```cpp
std::unique_ptr<Node>
std::unique_ptr<Edge>
```

### 2. Node 与 Edge 之间不互相拥有

Node 与 Edge 之间使用普通指针：

```cpp
Node*
Edge*
Rule*
```

这些是非拥有型指针，只用于建立关系，不负责释放对象。

调用者不能执行：

```cpp
delete main_cpp;
delete edge;
```

因为它们由 `BuildGraph` 中的 `unique_ptr` 管理。

---

## 四、Node 唯一性

`BuildGraph::get_or_create_node()` 应保证：

```text
同一路径 -> 同一个 Node 对象
```

示例：

```cpp
auto* node1 = graph.get_or_create_node("main.cpp");
auto* node2 = graph.get_or_create_node("main.cpp");

assert(node1 == node2);
```

不能只比较路径字符串，因为两个不同对象也可能保存相同路径。

必须比较指针地址，才能证明它们是同一个实体。

---

## 五、图的双向一致性

构造：

```text
main.cpp --compile--> main.o
```

需要同时满足：

### Edge 看 Node

```text
compile_edge.inputs  包含 main.cpp
compile_edge.outputs 包含 main.o
```

### Node 看 Edge

```text
main.cpp.out_edges 包含 compile_edge
main.o.in_edge 指向 compile_edge
```

因此不能只调用：

```cpp
edge->add_input(node);
edge->add_output(node);
```

否则可能只更新 Edge 一侧。

应该由 `BuildGraph` 统一维护：

```cpp
void BuildGraph::add_input(Edge* edge, Node* node)
{
    edge->add_input(node);
    node->add_out_edge(edge);
}

void BuildGraph::add_output(Edge* edge, Node* node)
{
    if (node->in_edge() != nullptr) {
        throw std::runtime_error(
            "node already has a producer: " + node->path()
        );
    }

    edge->add_output(node);
    node->set_in_edge(edge);
}
```

---

## 六、图结构不变量

### 1. 同一路径只对应一个 Node

```text
main.cpp -> 唯一 Node
```

否则 Dirty 状态和依赖传播可能落到不同对象上。

### 2. 一个输出最多一个生产者

非法情况：

```text
Edge A -> main.o
Edge B -> main.o
```

如果允许，系统无法确定 `main.o` 应由哪个 Edge 生成。

因此：

```cpp
if (node->in_edge() != nullptr) {
    throw std::runtime_error(...);
}
```

### 3. Edge 与 Node 的关系必须双向一致

输入关系：

```text
Edge.inputs 包含 Node
Node.out_edges 包含 Edge
```

输出关系：

```text
Edge.outputs 包含 Node
Node.in_edge 指向 Edge
```

---

## 七、完整 DAG 验证

最终图：

```text
main.cpp ──compile──> main.o ──┐
                               ├──link──> app
math.cpp ──compile──> math.o ──┘
```

### main.cpp

```text
in_edge = nullptr
out_edges = [compile_main]
```

### main.o

```text
in_edge = compile_main
out_edges = [link_app]
```

### app

```text
in_edge = link_app
out_edges = []
```

### compile_main

```text
rule = compile_rule
inputs = [main.cpp]
outputs = [main.o]
```

### link_app

```text
rule = link_rule
inputs = [main.o, math.o]
outputs = [app]
```

---

## 八、使用 assert 自动验证

示例：

```cpp
assert(main_cpp_again == main_cpp);

assert(compile_main->inputs().size() == 1);
assert(compile_main->inputs().at(0) == main_cpp);

assert(main_o->in_edge() == compile_main);
assert(main_o->out_edges().at(0) == link_app);

assert(link_app->inputs().size() == 2);
assert(app->in_edge() == link_app);
assert(app->out_edges().empty());
```

`assert(condition)`：

- 条件为真：继续执行
- 条件为假：程序立即终止并报告错误

Demo 中使用 `assert` 可以自动检查图结构，不必只依赖肉眼看输出。

---

## 九、今天遇到的 C++ 编译问题

### 1. 前向声明与完整类型

前向声明：

```cpp
class Node;
class Edge;
```

只告诉编译器这些类型存在。

可以使用：

```cpp
Node*
Node&
Edge*
```

不能使用：

```cpp
Node node;
node->path();
sizeof(Node);
```

调用成员函数或创建对象时，必须包含完整定义：

```cpp
#include "forge/node.hpp"
#include "forge/edge.hpp"
```

### 2. unique_ptr 与不完整类型

头文件中可以声明：

```cpp
std::unique_ptr<Node>
std::unique_ptr<Edge>
```

但析构这些对象时，编译器必须看到完整类型。

因此在头文件中声明析构函数：

```cpp
~BuildGraph();
```

在 `.cpp` 中包含完整头文件后定义：

```cpp
BuildGraph::~BuildGraph() = default;
```

这样析构代码会在知道 `Node`、`Edge` 完整定义的位置生成。

### 3. include 引号错误

错误：

```cpp
#include "forge/rule.hpp
```

缺少结束引号会导致：

```text
missing terminating " character
#include expects "FILENAME"
```

后续的 `Rule incomplete type` 是连锁错误。

排错原则：

> 优先修复最前面的错误，后面的错误可能会自动消失。

### 4. 编译错误与链接错误

#### 编译错误

例如：

```text
invalid use of incomplete type
```

说明单个 `.cpp` 无法被正确编译成 `.o`。

#### 链接错误

例如：

```text
undefined reference to forge::Rule::Rule(...)
```

说明：

- 声明存在
- 调用代码已经生成
- 但链接器找不到对应实现

常见原因：

- `.cpp` 没有实现函数
- 声明和定义签名不一致
- 命名空间不一致
- `.cpp` 没加入 CMake 源文件列表

可用：

```bash
nm -C build/debug/libforge_core.a | grep 'forge::Rule'
```

查看静态库中的 C++ 符号。

---

## 十、C++ 构建流程

本项目的构建链路：

```text
rule.cpp
node.cpp
edge.cpp
build_graph.cpp
        ↓ 编译
对应的 .o
        ↓ 归档
libforge_core.a

graph_demo.cpp
        ↓ 编译
graph_demo.cpp.o

graph_demo.cpp.o + libforge_core.a
        ↓ 链接
graph_demo
```

### 编译

```text
.cpp -> .o
```

每个源文件单独编译。

### 静态库

```text
多个 .o -> libforge_core.a
```

### 链接

```text
graph_demo.cpp.o + libforge_core.a -> graph_demo
```

---

## 十一、增量构建观察

首次完整构建后，不修改文件再次执行：

```bash
cmake --build build/debug
```

预期：

```text
ninja: no work to do.
```

只修改 `tests/graph_demo.cpp` 后，通常只有：

```text
[1/2] Building CXX object ... graph_demo.cpp.o
[2/2] Linking CXX executable graph_demo
```

核心库没有变化，因此无需重新编译。

这正是增量构建系统的价值：

```text
只重新构建受变化影响的目标
```

---

## 十二、工程目录

```text
ForgeBuild/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── docs/
│   └── day2-build-graph.md
├── include/forge/
│   ├── build_graph.hpp
│   ├── edge.hpp
│   ├── node.hpp
│   └── rule.hpp
├── src/
│   ├── build_graph.cpp
│   ├── edge.cpp
│   ├── node.cpp
│   └── rule.cpp
└── tests/
    └── graph_demo.cpp
```

目录职责：

```text
include/  对外接口
src/      具体实现
tests/    测试和演示
docs/     学习与设计文档
build/    可重新生成的构建产物
```

---

## 十三、Git 提交

Day2 提交：

```bash
git commit -m "feat: implement core build graph model"
```

提交结果：

```text
commit: 1eae952
branch: main
files: 13
```

提交完成后检查：

```bash
git status
```

理想结果：

```text
nothing to commit, working tree clean
```

---

## 十四、Day2 最终成果

已完成：

- `Rule` 数据模型
- `Node` 数据模型
- `Edge` 数据模型
- `BuildGraph` 所有权管理
- Node 路径唯一性
- Node / Edge 双向连接
- 重复生产者检查
- 五 Node、三 Edge、两 Rule 的完整 DAG
- `assert` 自动验证
- CMake 静态库与可执行程序
- README 和设计文档
- Git 根提交

---

## 十五、需要能够独立回答的问题

### 为什么两个编译任务是两个 Edge？

因为它们是两个具体构建实例，输入和输出不同。

### 为什么它们可以共享一个 Rule？

因为它们使用相同命令模板。

### 为什么 main.o 同时有 in_edge 和 out_edges？

因为它由编译 Edge 生成，又被链接 Edge 使用。

### 为什么源文件的 in_edge 是 nullptr？

因为源文件是外部输入，不是由 ForgeBuild 生成。

### 为什么 app 的 out_edges 为空？

因为它是当前构建图的最终产物。

### 为什么需要双向引用？

- Edge 到 Node：获取构建输入和输出
- Node 到 Edge：查找生产者和消费者，支持后续 Dirty 传播

### 为什么 BuildGraph 使用 unique_ptr？

因为它是 Node 和 Edge 的唯一所有者，能够自动安全释放对象。

---

## 十六、Day3 预告

Day2 中构建图是手动创建的：

```cpp
graph.get_or_create_node("main.cpp");
graph.create_edge(&compile_rule);
```

Day3 将开始解析类似文本：

```text
rule compile
  command = g++ -c $in -o $out

build main.o: compile main.cpp
```

自动生成：

```text
Rule + Node + Edge + BuildGraph
```

Day3 主题：

- 设计最小 Forgefile 语法
- 行读取与空白处理
- Rule 解析
- Build 语句解析
- 文本到 DAG 的转换
- 解析错误信息
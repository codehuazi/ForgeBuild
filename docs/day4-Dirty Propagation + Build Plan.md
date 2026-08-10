ForgeBuild Day4 学习笔记（开发过程版）

日期：Day4

主题：

实现增量构建分析系统（Dirty Propagation + Build Plan）

一、今日目标

Day3结束：

已经完成：

build.forge

        |
        v

Parser

        |
        v

Manifest

        |
        v

BuildGraph


可以解析：

rule compile

command = g++ -c

build main.o : compile main.cpp

生成：

main.cpp

    |
 compile

    |

main.o


但是还缺少：

如果：

main.cpp 被修改

系统不知道：

哪些目标需要重新生成？
哪些命令需要执行？

所以 Day4 实现：

修改检测

↓

dirty传播

↓

寻找需要构建的Edge

↓

生成BuildPlan

二、遇到问题1：Edge中调用Node方法失败
错误

编译：

error: invalid use of incomplete type ‘class forge::Node’

位置：

edge.cpp

代码：

if(input->dirty())
{
}
原因

之前：

edge.hpp：

class Node;

这是：

前向声明。

目的：

避免头文件循环依赖。

但是：

前向声明只能声明：

“有这么一个类”

不能调用：

node->dirty()
node->exists()

因为编译器不知道Node里面有什么。

错误结构：
edge.hpp

class Node;


Edge
 |
 Node*


只能保存指针。

但是：

edge.cpp：

需要：

Node成员函数
解决

在：

src/edge.cpp

增加：

#include "forge/node.hpp"

最终：

#include "forge/edge.hpp"

#include "forge/node.hpp"
#include "forge/rule.hpp"

学到：

C++工程中：

头文件

只需要知道：

class Node;
cpp文件

需要使用：

#include "node.hpp"

这是大型C++项目常用做法。

三、实现Edge::needs_build()

目标：

判断一个构建任务是否需要执行。

设计：

Edge：

输入：

main.cpp

输出：

main.o

判断：

1. 输入变化
if(input->dirty())

例如：

main.cpp修改


需要重新编译。

2. 输出不存在
if(!output->exists())

第一次构建：

main.o不存在


需要生成。

3. 输出dirty

例如：

上游传播：

main.o dirty


继续影响下游。

最终：

bool Edge::needs_build() const
四、遇到问题2：Builder新增后没有真正执行传播

第一次修改Builder：

BuildPlan Builder::build()
{

    auto edges =
        collect_dirty_edges();

}

运行：

Plan edges:1
main.o dirty:0

发现：

dirty没有传播。

分析原因

之前：

Builder负责：

传播dirty

现在改成：

只收集Edge

导致流程：

错误：

main.cpp dirty

↓

collect

↓

结束


缺少：

main.cpp

↓

main.o

正确流程

必须：

第一步：

dirty传播


第二步：

收集Edge


第三步：

生成BuildPlan


所以：

Builder::build：

应该：

propagate_dirty()

        ↓

collect_dirty_edges()

        ↓

BuildPlan

五、实现Dirty传播

目标：

例如：

main.cpp
     |
 compile
     |
main.o


修改：

main.cpp dirty=true

希望：

main.o dirty=true


实现：

void Builder::propagate_dirty(Node* node)

逻辑：

Node

 ↓

找到out_edges

 ↓

Edge

 ↓

找到outputs

 ↓

标记dirty


代码：

for(auto* edge : node->out_edges())
{

    if(!edge->needs_build())
    {
        continue;
    }


    for(auto* output : edge->outputs())
    {
        output->mark_dirty();

        propagate_dirty(output);
    }

}


测试：

输出：

propagate: main.cpp
propagate: main.o


说明成功。

六、遇到问题3：Builder是否应该直接访问edges_

讨论：

问题：

Builder需要遍历所有Edge：

graph_.edges()

但是：

BuildGraph：

private:

std::vector<Edge*> edges_;

我的设计考虑

不能：

把：

edges_

改public。

原因：

破坏封装。

Builder需要：

读取。

不需要：

修改。

所以设计：

增加：

const std::vector<std::unique_ptr<Edge>>& edges() const;


提供只读接口。

学习：

这是C++工程常见设计：

private数据

↓

const getter

↓

外部只读访问

七、遇到问题4：测试代码变量错误

错误：

main_o->dirty()

但是实际变量：

obj

编译：

main_o was not declared

原因：

测试代码复制之前graph_demo变量。

解决：

统一：

obj

或者：

main_o

保持命名一致。

学习：

测试代码也是工程代码。

不要随便复制demo。

八、BuildPlan设计

之前：

Builder：

直接输出：

cout

问题：

未来无法扩展：

执行
并行
缓存

所以增加：

BuildPlan

结构：

Builder

  |
  v

BuildPlan

  |
  v

Executor


BuildPlan保存：

vector<Edge*>

为什么不是Node？

因为：

Node只是文件。

真正执行：

是Edge。

例如：

Node:

main.cpp


不能执行。

Edge:

g++ -c main.cpp -o main.o


才是任务。

九、BuildPlan输出验证

增加：

plan.print();


最终：

运行：

./build/debug/builder_demo

输出：

propagate: main.cpp
propagate: main.o

===== Build Plan =====
main.cpp --compile--> main.o
======================

Plan edges: 1
main.o dirty:1

十、今天新增文件

Day4新增：

include/forge/builder.hpp

include/forge/build_plan.hpp


src/builder.cpp

src/build_plan.cpp


tests/builder_demo.cpp

十一、Day4最终架构

现在：

                 Manifest

                     |

                     |

               BuildGraph

                     |

        -----------------------

        |                     |

      Node                  Edge


        |

        |

     Builder


        |

        |

   BuildPlan


十二、今天掌握的工程思想
1. 增量构建核心

不是：

全部重新编译


而是：

变化传播

↓

局部重新构建

2. 前向声明与include关系

头文件：

减少依赖。

cpp：

提供完整定义。

3. 状态和行为分离

Node：

状态：

dirty
exists


Edge：

行为：

输入
输出
规则


Builder：

分析。

4. 中间表示思想

BuildPlan就是：

Intermediate Representation。

类似：

编译器：

源码

↓

AST

↓

IR

↓

机器码


构建系统：

BuildGraph

↓

BuildPlan

↓

执行

Day4完成状态
✅ Node dirty管理

✅ Edge构建判断

✅ Dirty传播

✅ 增量分析

✅ BuildPlan

✅ Builder职责拆分


ForgeBuild当前能力：

已经具备最小Ninja核心
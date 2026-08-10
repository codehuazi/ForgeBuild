ForgeBuild Day3 学习笔记
主题：从 Forgefile 到 BuildGraph —— 构建描述解析
一、Day3目标

Day2 完成：

Node
Edge
Rule
BuildGraph

这些数据结构。

但是此时：

Node
Edge
Rule

都是手动创建的。

例如：

auto* node =
    graph.get_or_create_node("main.cpp");

真实构建系统不能这样。

用户输入的是：

rule compile

command = g++ -c $in -o $out


build main.o: compile main.cpp

因此 Day3 目标：

实现：

Forgefile

    ↓

Parser

    ↓

Manifest

    ↓

BuildGraph

    ↓

DAG
二、为什么需要 Manifest？
1. Manifest是什么？

Manifest表示：

一次构建描述文件解析后的完整状态。

类似 Ninja：

build.ninja

        ↓

Manifest

        ↓

State

ForgeBuild：

Manifest

|
|-- Rules
|
|-- BuildGraph


其中：

Rules

保存：

rule compile

例如：

compile

command:
g++ -c $in -o $out

BuildGraph

保存：

Node

Edge


也就是：

真正的依赖关系。

三、Parser职责

Parser负责：

把文本转换成结构。

例如：

输入：

rule compile

转换：

Rule对象

输入：

build main.o: compile main.cpp

转换：

Edge

inputs:

    main.cpp


outputs:

    main.o


rule:

    compile


所以：

Parser不负责：

执行命令
编译文件
调度任务

它只负责：

建图。

四、Rule解析
输入
rule compile

command = g++ -c $in -o $out
解析流程
文本

↓

Parser::parse_line()

↓

发现:

rule

↓

Manifest.add_rule()

↓

创建 Rule


内存：

Manifest


rules_

{

    "compile"

          |

          ↓

        Rule

}

五、build语句解析

输入：

build main.o: compile main.cpp
第一步：分割冒号

原始：

main.o: compile main.cpp


分成：

output：

main.o

rest：

compile main.cpp
第二步：split

调用：

split(
    rest,
    ' '
);

得到：

[
    "compile",
    "main.cpp"
]


含义：

parts[0]

↓

Rule名字


parts[1]

↓

输入文件

六、根据名字查找 Rule

代码：

Rule* rule =
    manifest.find_rule(
        parts[0]
    );

作用：

通过：

compile

找到：

Rule对象

为什么不能重新创建 Rule？

错误：

new Rule("compile")

原因：

Rule应该由 Manifest 统一管理。

否则：

可能出现：

Rule1

compile


Rule2

compile


两个规则。

七、创建 Edge

找到 Rule 后：

创建：

Edge* edge =
    manifest.graph()
            .create_edge(rule);

为什么？

因为：

Rule不是一次构建。

例如：

Rule：

compile

代表：

如何编译cpp

但是：

Edge代表：

这一次具体编译

main.cpp

↓

main.o


关系：

Rule

  |
  | 创建

  ↓


Edge


  |
  |
  +------ Node
八、创建输出 Node

代码：

Node* output_node =
    manifest.graph()
            .get_or_create_node(
                output
            );


例如：

main.o

创建：

Node(main.o)


为什么不能直接new？

因为：

BuildGraph需要保证：

同一个文件只有一个Node。

例如：

错误：

Node1

main.o



Node2

main.o


两个对象表示同一个文件。

之后：

dirty分析会混乱。

九、连接 Edge 和 Node

输出：

edge->add_output(
    output_node
);

形成：

Edge

 |
 |
 ↓

main.o


输入：

edge->add_input(
    input_node
);


形成：

main.cpp

    |

    |

   Edge

    |

    |

main.o


最终：

内存中的 DAG：

                 Rule

              compile

                  |

                  ↓


              Edge


              /      \


             /        \


       main.cpp       main.o


          Node          Node

十、BuildGraph dump

为了验证图是否正确，实现：

dump()

输出：

===== Build Graph =====


Nodes:

main.cpp
main.o


Edges:

Edge

 Rule: compile

 Inputs:

    main.cpp


 Outputs:

    main.o


作用：

类似 Ninja：

ninja -t graph

用于观察构建图。

十一、今天遇到的重要C++知识
1. 前向声明

例如：

class Node;

只能使用：

Node*

不能：

node->path()


因为编译器不知道 Node 内部。

需要：

#include "node.hpp"

2. unique_ptr + incomplete type问题

错误：

invalid application of sizeof to incomplete type


原因：

unique_ptr<Node>

析构时：

需要知道：

Node完整大小。

解决：

hpp：

class Node;


cpp：

#include "node.hpp"


并把析构放cpp。

十二、Day3完成后的架构

现在：

                build.forge


                    |

                    ↓


                 Parser


                    |

                    ↓


                Manifest


          +----------------+

          |                |

          ↓                ↓


       Rules          BuildGraph


                         |

                         ↓


                  Node + Edge


                         |

                         ↓


                       DAG

十三、当前 ForgeBuild完成度
模块	状态
Rule	✅
Node	✅
Edge	✅
BuildGraph	✅
Manifest	✅
Parser读取文件	✅
rule解析	✅
build解析	✅
创建Edge	✅
创建Node	✅
生成DAG	✅
执行命令	❌
Dirty分析	❌
Scheduler	❌
十四、Day4预告

下一阶段：

Scheduler + Dirty Analysis

解决真正核心问题：

修改：

math.cpp

之后：

为什么不是：

重新编译全部cpp


而是：

math.cpp

↓

math.o

↓

app


会实现：

Node dirty

↓

找到out_edges

↓

判断Edge是否需要执行

↓

拓扑排序

↓

执行


也就是：

Ninja真正工作的部分。
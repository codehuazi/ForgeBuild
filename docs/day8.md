ForgeBuild 学习笔记 Day8：命令哈希与持久化增量构建
一、Day8 学习目标

Day7 已经完成：

文件存在性判断
时间戳比较
增量构建传播
DAG 拓扑排序
BuildPlan 生成
Executor 执行

但是还存在一个重要问题：

如果构建命令发生变化，而源文件时间没有变化，ForgeBuild 无法发现。

例如：

第一次：

g++ -c main.cpp -o main.o

后来修改编译参数：

g++ -O2 -c main.cpp -o main.o

此时：

main.cpp 时间没有变化
main.o 仍然存在

按照 Day7 的判断：

edge->needs_build()

可能返回：

false

导致错误：

不会重新编译

而真实构建系统（Ninja、Make）必须检测：

当前构建命令是否和上一次一致。

因此 Day8 引入：

Command Hash
BuildLog
持久化构建历史
Executor 更新日志
跨进程增量构建
二、整体架构变化

Day7：

          文件状态

             |
             v

        +---------+
        | Builder |
        +---------+

             |
             v

        BuildPlan

             |
             v

        Executor

Day8：

增加 BuildLog：

                 .forge_log
                     |
                     v

              +-------------+
              |  BuildLog   |
              +-------------+

                     |
                     |
                     v


+---------+      +---------+      +---------+
|  Node   | ---> |  Edge   | ---> |  Node   |
+---------+      +---------+      +---------+

                     |
                     v

              +-------------+
              |   Builder   |
              +-------------+

                     |
                     v

                BuildPlan

                     |
                     v

              +-------------+
              |  Executor   |
              +-------------+

                     |
                     v

              更新 BuildLog
三、第一部分：Command Hash（命令哈希）
3.1 为什么需要 Hash

构建系统不能直接比较完整字符串：

例如：

g++ -c main.cpp -o main.o

可能非常长：

compiler path
flags
environment
include path
macro
source file
output file

因此通常转换为：

command string
       |
       v
 hash function
       |
       v
 uint64_t

例如：

g++ -c main.cpp -o main.o

↓

14425504672841978440
3.2 Hash 的要求

需要满足：

相同命令

必须：

command1 == command2

hash1 == hash2

测试：

输出：

command1 hash:
14425504672841978440

command2 hash:
14425504672841978440

说明：

✅ 相同命令产生相同 hash

不同命令

例如：

普通编译：

g++ -c main.cpp -o main.o

优化编译：

g++ -O2 -c main.cpp -o main.o

输出：

command3 hash:
5497661614543725550

说明：

✅ 不同命令产生不同 hash

四、第二部分：BuildLog 内存结构
4.1 BuildLog 的作用

BuildLog 保存：

输出文件
    |
    v
历史构建命令 hash

例如：

main.o
    |
    v
14425504672841978440


app
    |
    v
14393057532728118009
4.2 数据结构

核心：

std::unordered_map<
    std::string,
    std::uint64_t
> entries_;

对应：

key                 value

main.o       --->   command hash

app          --->   command hash
五、BuildLog 接口设计
5.1 record()

作用：

记录一次成功构建。

接口：

void record(
    const std::string& output_path,
    uint64_t command_hash
);

例如：

build_log.record(
    "main.o",
    14425504672841978440
);

保存：

main.o → 14425504672841978440
5.2 contains()

判断是否存在历史记录。

bool contains(
    const std::string& output_path
) const;

例如：

contains("main.o")

返回：

true
5.3 command_hash()

获取历史 hash。

uint64_t command_hash(
    const std::string& output_path
);

例如：

main.o

返回：

14425504672841978440
5.4 command_matches()

最重要接口。

作用：

判断：

当前命令 hash

是否等于

历史命令 hash

例如：

command_matches(
    "main.o",
    current_hash
)

返回：

true

表示：

命令没有变化

返回：

false

表示：

需要重新构建
六、第三部分：BuildLog 磁盘持久化
6.1 为什么需要保存到磁盘

如果只存在：

BuildLog build_log;

程序退出：

entries_
消失

下一次启动：

没有历史记录

无法增量。

所以增加：

.forge_log
6.2 日志格式

采用：

输出路径<TAB>命令hash

例如：

app	14393057532728118009
main.o	14425504672841978440
6.3 save()

作用：

内存：

entries_

写入：

.forge_log

流程：

unordered_map

      |

      v

ofstream

      |

      v

.forge_log
6.4 load()

作用：

读取：

.forge_log

恢复：

entries_

流程：

.forge_log

      |

      v

解析每一行

      |

      v

unordered_map

      |

      v

BuildLog
七、第四部分：Builder 接入 BuildLog
7.1 修改前

Day7：

Builder 判断：

文件状态

例如：

输出不存在

或者

输入时间更新
7.2 修改后

Builder 判断：

需要构建 =

    文件状态过期

    OR

    命令发生变化

代码逻辑：

file_state_requires_build
||
command_requires_build
八、command_changed() 设计

新增：

bool command_changed(
    const Edge* edge
) const;

流程：

第一步

获取当前命令：

例如：

g++ -c main.cpp -o main.o
第二步

计算 hash：

hash_string(
    edge->command()
)

得到：

14425504672841978440
第三步

查询 BuildLog：

历史：

main.o
14425504672841978440

比较：

当前 hash

=

历史 hash

如果：

不同

返回：

true

表示：

命令变化，需要重新构建
九、第五部分：Executor 更新 BuildLog
9.1 为什么 Executor 更新

错误：

Builder 判断需要构建

↓

立即写日志

如果：

编译失败

日志却记录成功。

因此：

正确：

Executor

执行命令

↓

成功

↓

更新 BuildLog
9.2 执行成功后的流程

例如：

执行：

g++ -c main.cpp -o main.o

成功：

计算 hash

↓

main.o

↓

record()

得到：

main.o

14425504672841978440
十、完整一次构建流程
第一次构建

状态：

main.o 不存在

app 不存在

.forge_log 不存在

流程：

load(".forge_log")

        |

        v

BuildLog为空


        |

        v


Builder

发现：

输出不存在

        |

        v


生成：

compile
link


        |

        v


Executor执行


        |

        v


更新BuildLog


        |

        v


save(".forge_log")
第二次构建

状态：

main.o 存在

app 存在

.forge_log 存在

流程：

load(".forge_log")

        |

        v


Builder


检查文件：

正常


检查命令：

hash一致


        |

        v


BuildPlan:

0 Edge

输出：

plan edge count: 0
十一、实际测试记录
11.1 Hash 测试

运行：

./build/debug/hash_demo

结果：

command1 hash:
14425504672841978440

command2 hash:
14425504672841978440

command3 hash:
5497661614543725550

hash checks passed

结论：

✅ 命令哈希正确

11.2 BuildLog 内存测试

运行：

./build/debug/build_log_demo

结果：

old hash:
14425504672841978440

new hash:
5497661614543725550

build log memory checks passed

结论：

✅ 内存记录、查询、匹配正确

11.3 BuildLog 磁盘测试

结果：

build log memory and disk checks passed

验证：

save()
load()

均正确。

11.4 Builder 命令变化测试

运行：

./build/debug/command_change_demo

结果：

empty log plan count: 2

matching log plan count: 0

changed command plan count: 2

command change checks passed

验证：

空日志：

重新构建

匹配：

跳过

命令变化：

compile + link
11.5 Executor 更新日志测试

运行：

./build/debug/executor_build_log_demo

结果：

first plan count: 2

g++ -c main.cpp -o main.o

g++ main.o -o app


main.o hash recorded:
14425504672841978440


app hash recorded:
14393057532728118009


second plan count: 0


executor build log checks passed

验证：

Executor 成功执行后：

更新BuildLog
11.6 完整跨进程测试

清理：

rm -f main.o app .forge_log

第一次：

./build/debug/incremental_chain_demo

结果：

plan edge count: 2

g++ -c main.cpp -o main.o

g++ main.o -o app

execute result: true

查看日志：

cat .forge_log

得到：

app     14393057532728118009
main.o  14425504672841978440

第二次：

./build/debug/incremental_chain_demo

结果：

plan edge count: 0

execute result: true

结论：

✅ 跨进程增量构建完成

十二、Day8 完成情况总结
功能	状态
命令字符串 Hash	✅
BuildLog 内存结构	✅
日志保存 save()	✅
日志读取 load()	✅
Builder 查询日志	✅
命令变化检测	✅
Executor 更新日志	✅
跨进程增量构建	✅
十三、Day8 学到的工程思想
1. 构建系统不是只看文件

真实构建条件：

输入文件
+
文件时间
+
构建命令
+
环境信息
2. 状态记录与执行分离

Builder：

决定做什么

Executor：

真正执行

BuildLog：

记录历史状态

职责清晰。

3. 成功后提交状态

核心思想：

执行成功

↓

更新状态

失败不能污染缓存。

Day8 最终成果

现在 ForgeBuild 已经具备类似 Ninja 的核心增量能力：

        ForgeBuild

             |
             v

     DAG + BuildPlan

             |

     文件状态检查

             |

     命令Hash检查

             |

       BuildLog

             |

     Executor执行

             |

      持久化历史

下一阶段 Day9 可以进入：

并行调度器 Scheduler

开始实现：

Worker Thread
就绪队列
DAG 并行执行
-j N 参数
失败传播
ForgeBuild 学习笔记：C++ 多线程与锁机制

版本：Day9 Scheduler 并行调度阶段

1. 为什么需要多线程？
1.1 单线程构建的问题

最开始 ForgeBuild：

compile a.cpp

↓

compile b.cpp

↓

compile main.cpp

↓

link app

所有任务一个接一个执行。

但是实际工程：

a.cpp
b.cpp
main.cpp

三个编译任务互相没有依赖。

理论上：

Worker1:
compile a.cpp

Worker2:
compile b.cpp

Worker3:
compile main.cpp

同时执行。

这样可以降低总构建时间。

2. 多线程带来的问题

多个线程共享进程资源。

例如：

int counter = 0;

两个线程：

线程1：

counter++;

线程2：

counter++;

看起来应该：

0 → 1 → 2

但是实际上：

counter++

不是一个原子操作。

它包含：

读取 counter

↓

加1

↓

写回 counter

可能：

线程1:
读取 0

线程2:
读取 0

线程1:
写入 1

线程2:
写入 1

最终：

counter = 1

而不是：

counter = 2

这叫：

数据竞争（Data Race）
3. 什么是数据竞争？

定义：

多个线程同时访问同一块内存，并且至少一个线程会修改它，没有同步机制。

三个条件：

条件1

多个线程。

例如：

std::thread
条件2

访问同一数据。

例如：

entries_

或者：

ready_queue_
条件3

至少一个线程修改。

例如：

写：

entries_["a.o"]=123;

读：

entries_.find("a.o");

满足三个条件：

就是数据竞争。

4. mutex（互斥锁）
4.1 mutex 是什么？

mutex:

mutual exclusion

中文：

互斥锁

作用：

保证：

同一时间只有一个线程进入临界区。

例如：

std::mutex mutex;

想访问共享数据：

mutex.lock();

修改数据;

mutex.unlock();
5. 临界区（Critical Section）

临界区：

访问共享资源的代码区域。

例如：

entries_[path] = hash;

因为：

entries_

是共享数据。

所以：

mutex.lock();

entries_[path]=hash;

mutex.unlock();

中间部分：

就是临界区。

6. lock_guard（RAII方式加锁）

不推荐：

mutex.lock();

do something;

mutex.unlock();

原因：

如果：

return;

或者：

throw exception;

可能忘记解锁。

例如：

mutex.lock();

if(error)
{
    return;
}

mutex.unlock();

这里：

return 后锁永远存在。

其他线程永久等待。

C++推荐：

std::lock_guard<std::mutex> lock(
    mutex
);

原理：

创建对象：

lock_guard构造
        |
        ↓
自动lock

离开作用域：

lock_guard析构
        |
        ↓
自动unlock

这就是：

RAII

(Resource Acquisition Is Initialization)

资源获取即初始化。

7. unique_lock

unique_lock：

比 lock_guard 更灵活。

例如：

std::unique_lock<std::mutex> lock(
    mutex
);

支持：

手动释放
lock.unlock();
条件变量

后面 Scheduler 用：

condition.wait(
    lock
);

因为条件变量需要：

加锁；
睡眠；
被唤醒；
自动重新获得锁。

lock_guard 做不到。

8. condition_variable（条件变量）

问题：

如果 Worker 没任务：

不能：

while(true)
{
    check queue;
}

这样：

CPU 100%。

这种叫：

忙等待（Busy Waiting）

正确：

让线程睡眠。

例如：

condition.wait(lock);

线程：

没有任务

↓

睡眠

↓

任务来了

↓

唤醒
9. wait 的条件判断

ForgeBuild：

condition_.wait(
    lock,
    [this]()
    {
        return stopping_
            || !ready_queue_.empty();
    }
);

意思：

线程等待：

两个条件之一成立：

情况1

有任务：

!ready_queue_.empty()

Worker 工作。

情况2

退出：

stopping_

Worker结束。

10. 为什么 wait 需要 lambda？

因为线程醒来不代表条件一定满足。

可能：

虚假唤醒；
其他线程抢走任务。

所以：

wait()

内部逻辑类似：

while(!condition)
{
    sleep();
}
11. 多把锁的设计

ForgeBuild 现在有两把锁。

mutex_

保护：

调度状态

包括：

ready_queue_

indegree_

running_edges_

stopping_

failed_

例如：

ready_queue_.pop();

必须加：

mutex_
output_mutex_

保护：

日志输出

例如：

std::cout

因为多个线程同时输出：

可能：

worker worker 1 started

乱掉。

所以：

output_mutex_

只负责：

cout
12. 为什么不能所有东西用一把锁？

错误设计：

mutex_
保护：

queue

+

cout

+

BuildLog

+

问题：

锁范围过大。

例如：

Worker:

拿锁

执行g++

打印

更新日志

释放锁

其他 Worker：

全部等待。

结果：

多线程退化成单线程。

正确：

不同资源不同锁：

Scheduler状态
      |
   mutex_


日志
      |
 output_mutex_


BuildLog
      |
 BuildLog::mutex_
13. 死锁（Deadlock）

死锁：

线程互相等待。

例：

线程A：

拿 mutex_A

等待 mutex_B

线程B：

拿 mutex_B

等待 mutex_A

结果：

A等B
B等A

永远停止。

避免：

规则1

固定加锁顺序。

例如：

永远：

mutex_
 ↓
output_mutex_

不要：

有时：

mutex_
output_mutex_

有时：

output_mutex_
mutex_
规则2

减少同时持有多个锁。

例如：

错误：

lock mutex_

cout

unlock

正确：

lock mutex_

取数据

unlock


lock output_mutex_

打印

unlock
14. BuildLog 为什么需要锁？

BuildLog：

unordered_map entries_

多个 Worker：

Worker1:
record(a.o)


Worker2:
record(b.o)

同时修改。

必须：

std::lock_guard<std::mutex>

保护。

15. 为什么只读也需要锁？

错误理解：

读取不用锁。

正确：

如果存在：

线程A 写

线程B 读

仍然危险。

例如：

线程A：

entries_.insert()

线程B：

entries_.find()

可能：

扩容；
修改内部结构。

所以：

读也需要同步。

16. save 为什么不用一直持锁？

错误：

lock();

写文件;

unlock();

文件IO慢。

其他线程：

record()
等待

时间很长。

正确：

lock

复制snapshot

unlock


写文件

减少锁占用时间。

17. 当前 ForgeBuild 多线程架构

现在：

              Scheduler

                 |
              mutex_

                 |
        -----------------
        |               |

 ready_queue       indegree


                 |
                 v


            Worker线程


                 |
                 v


              Executor


                 |
                 v


             BuildLog

                 |
              mutex_
18. 当前学习阶段总结

你现在已经掌握：

✅ 为什么需要线程
✅ 什么是数据竞争
✅ 为什么需要 mutex
✅ lock_guard RAII
✅ unique_lock
✅ condition_variable
✅ wait 原理
✅ 多锁设计
✅ 死锁原因
✅ 临界区设计
✅ BuildLog 为什么线程安全

下一步 Scheduler 真正并行化时，会把这些知识全部结合起来：

Worker领取任务

↓

Executor执行命令

↓

更新BuildLog

↓

finish_edge()

↓

唤醒等待线程

↓

继续执行下游任务

这部分就是 Ninja / Bazel / 编译系统真正核心的调度器设计。你现在已经完成了前置知识铺垫。

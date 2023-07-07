# Muduo

- **开发环境：** Ubuntu 22.04.1
- **涉及技术：** C++，多线程编程，Socket网络编程

## 项目简介

参考陈硕《Linux多线程服务端编程》，基于**C++11**实现muduo网络库的核心功能**TcpServer**。

- 模拟 muduo 库实现non-blocking + IO-multiplexing + loop线程模型的高并发 TCP 服务器模型

- 实际使用方式延续了muduo库的使用方式，使用时只需要实现一系列包含目标操作的事件回调并配置一些基础信息，如：端口，名称等。
- 使用C++11的新语法：智能指针、function函数对象、lambda表达式、bind绑定器、并发库（thread , atomic,mutex,condition_variable ）等对muduo库的核心代码进行改写。

## 运行

- 生成动态链接库

```bash
cd Muduo
cmake .
make
```

- 将编译生成的 libMuduo.so 动态链接库拷贝到系统目录下

```bash
# 如果存在权限问题需要修改权限
sudo ./build.sh
```

- 运行demo

```bash
cd example
make
./testserver2
```

## 文件结构

```bash
.
├── Acceptor.cc               # 封装listenfd的相关操作
├── Acceptor.h
├── Buffer.cc                 # 读写缓冲区
├── Buffer.h
├── build.sh                  # 将生成的libMuduo.so放到头文件检索的路径下
├── Callbacks.h
├── Channel.cc                # 封装fd及fd感兴趣的事件, 以及根据事件执行的相应的回调
├── Channel.h
├── CMakeLists.txt            # 项目构建
├── CurrentThread.cc          # 获取当前线程的tid
├── CurrentThread.h
├── DefaultPoller.cc          # 选取使用Poll/EPoll
├── EPollPoller.cc            # 实现epoll多路转接模型
├── EPollPoller.h
├── EventLoop.cc              # Reactor反应堆: 事件循环
├── EventLoop.h
├── EventLoopThread.cc        # 事件循环线程, 提供启动底层的线程
├── EventLoopThread.h
├── EventLoopThreadPool.cc    # 事件循环线程池
├── EventLoopThreadPool.h
├── example                   # 例子
│   ├── Makefile
│   ├── testserver2
│   └── testserver.cc         # 一个回显服务器的demo
├── InetAddress.cc            # IP地址相关操作
├── InetAddress.h
├── lib
│   └── libMuduo.so           # Muduo的动态链接库
├── Logger.cc                 # 日志相关
├── Logger.h
├── noncopyable.h             # 禁止派生类对象进行拷贝构造和赋值操作的基类
├── Poller.cc                 # muduo库中多路事件分发器的核心IO复用模块
├── Poller.h
├── Socket.cc                 # 封装socket的相关操作
├── Socket.h
├── TcpConnection.cc          # 新用户连接后的一系列回调操作
├── TcpConnection.h
├── TcpServer.cc              # 所有组件的总调度
├── TcpServer.h
├── Thread.cc                 # 封装线程的相关操作
├── Thread.h
├── Timestamp.cc              # 获取时间相关的信息
└── Timestamp.h
```

## 项目分析

**Logger组件：** 对日志信息的封装，便于查看运行时状态，便于调试

- LOG_INFO：提示信息。
- LOG_ERROR：错误信息(不影响程序正常执行)。
- LOG_FATAL：致命错误信息(导致程序崩溃，执行了exit)。
- LOG_DEBUG：调试信息。

**Channel组件：** 封装fd，events，revents，以及一组回调

- fd：Poller上注册的文件描述符
- events：fd感兴趣的事件（读事件或者写事件）
- revents：Poller最终给Channel通知的这个fd上发生的事件，Channel根据相应的发生的事件来执行相应的回调

- 功能：
  - 更新Epoll中用户关心的事件
  - 采用bind绑定器和functional函数对象机制提供了设置用户关心事件发生后的回调处理函数
  - 当有用户感兴趣的事件发生时，根据发生事件的不同，调用用户事先设定的回调函数，处理事件
- 总结：Channel其实就是将Event以及Handler打包后的产物，用sockfd进行标识，同时提供了一些函数接口，便于后续操作。

### Poller & EPollPoller >> Demultiplex

**Poller组件：** 多路事件分发器的核心，可以看作Poll和Epoll的一个抽象

- 成员变量`channels_`：std::unordered_map<int, Channel *>
  - key：int类型，也就是Channel打包的sockfd
  - value：Channel* 类型，包含fd对应的channel
  - 含义：Poller检测到哪个fd有事件发生，通过fd作为key和这个map。快速找到对应的channel，这个channel里记录着详细的对应事件的回调方法

- 采用多态机制，提供抽象类，Poll和Epoll通过继承Poller，同时重写Poller的纯虚函数的接口，实现多路转接模型。

**EpollPoller组件：** 封装了Epoll的相关接口

- 通过继承Poller并且重写其提供的纯虚函数接口，实现Epoll多路转接模型
- 将Epoll的接口封装成了一个类，提供向Epoll中添加、修改、删除所关心的事件的接口以及开启事件监听的函数接口
- 对外还提供了向用户返回发生事件的接口以及更新Channel的接口

### EventLoop >> Reactor

**EventLoop组件：** 事件循环，也就是Reactor反应堆

- 整体上看，EventLoop管理的是一堆Channel和一个Poller，以及本身的wakeupFd_

- 由于Poller和Channel之间不能互相访问，因此EventLoop穿插在EpollPoller和Channel之间，Channel调用EventLoop提供的函数接口将用户所感兴趣的事件注册到Epoll上，还可以通过EventLoop对用户所感兴趣的事件在Epoll上进行更新、删除等操作
- 当有事件发生时，EpollPoller就会将所发生的事件通过参数反馈给EventLoop，EventLoop再调用用户预先在Channel中设置的回调函数EventHandler对发生事件进行处理
- 如果当前所发生的事件是运行在其他线程上的loop的所监听的事件，就需要唤醒阻塞epoll_wait上的线程，执行相应的EventHandler操作
- 在EventLoop的构造函数中，通过系统调用接口eventfd创建一个wakeupFd，并打包成Channel注册到所在线程的EpollPoller中
- 如果调用回调的loop所属线程不是当前线程，那么就向这个loop所属的wakeupFd对应的事件中随便写入一点数据，用以唤醒loop所在的线程执行相应的回调

> 因此：
>
> - Channel想把自己注册到Poller上，或者是在Poller上修改自己感兴趣的事件，都是通过Eventloop获取poller的对象，来向poller设置
> - 同样的，如果Poller监测到有sockfd有相应的事件发生，就通过Eventloop调用Channel相应的fd所发生事件的回调函数

### Thread & EventLoopThread & EventLoopThreadPool

**Thread组件：** 线程的相关操作

- 封装了线程的创建，启动，和执行线程的入口的函数
- 封装了线程的join函数

**EventLoopThread组件：** 事件循环线程

- 对Thread组件的封装，提供启动底层的线程的方法
- 为将要创建的线程设置入口函数

**EventLoopThreadPool组件：** 事件循环线程池

- 可以通过成员函数设置线程数量
- 根据线程数量创建线程，将每个线程上所创建的loop地址返回
- 通过轮询算法选择一个loop，将新连接用户所关心的Channel分发给loop
- 成员函数`getNextLoop`：默认是通过轮询算法获取下一个subloop，如果没有创建subloop，getNextloop永远返回的是baseloop

### Socket & Acceptor组件

**Socket组件：** 封装socket的相关操作，看作是网络编程函数接口的抽象

**Acceptor组件：** 封装了listenfd相关的操作

- 创建用于监听和获取新连接的accpetSocket，为其绑定地址信息
- 将accpetSocket以及其感兴趣的事件(新连接到来)打包成accpetChannel
- 监听新连接到来并将accpetChannel通过EventLoop注册到所属的epoll中去
  - acceptorChaneel所属的loop就是mainLoop

**buffer组件：** 数据缓冲区

- 解决TCP接受缓冲区较小但是用户发送数据过多问题，或是只需要读取部分数据的问题
- 缓冲区模型如下
  - prependable有8个字节，是头部信息
  - 如果缓冲区大小不够，会调用Buffer::makeSpace()对缓冲区进行扩容

```bash
+-------------------+----------------+----------------+ 
| prependable bytes | readable bytes | writable bytes |
|                   |    (CONTENT)   |                |
+-------------------+----------------+----------------+ 
|                   |                |                |
0       <=     readerIndex   <=  writerIndex   <=    size
```

**TcpConnection组件：** 新用户连接后的一系列回调操作

- 一个连接成功的客户端对应一个TcpConnection，包括：
  - 封装的socket，Channel
  - 各种回调
  - 高水位线 highWaterMark 的控制（不要发送过快）
  - 发送和接收缓冲区
- 功能：
  - 引入的bind绑定器和functional函数对象机制，为当前连接所对应的channel设置各种事件的回调函数
  - 实现EventHandler
  - 发送数据

**TcpServer组件：** 总调度

- 供用户创建TcpServer对象，使用TCP服务器
- 对外提供了设置建立新连接回调函数、读写消息回调函数、开启事件循环、设置线程数量并创建线程等函数接口
- 对内提供并绑定了有新用户连接时的回调函数，通过轮询算法分发新连接用户到EventLoopThread上(即subLoop)，并且设置了关闭连接的回调函数

## 如何使用

- 编写一个目标的Server类：
  - 在构造函数中绑定连接建立和断开的回调函数以及可读写事件的回调函数
  - 设置线程个数(一般和CPU核心数相同)
  - 使用start()接口启动服务
  - 实现连接建立和断开的回调函数以及可读写事件的回调函数
  - 实现可读写事件的回调

- 编写main函数：创建事件循环对象，提供InetAddress地址信息，创建TcpServer服务器对象，启动服务，开启事件循环

```shell
  __  __           _
 |  \/  |         | |
 | \  / |_   _  __| |_   _  ___
 | |\/| | | | |/ _` | | | |/ _ \
 | |  | | |_| | (_| | |_| | (_) |
 |_|  |_|\__,_|\__,_|\__,_|\___/
```


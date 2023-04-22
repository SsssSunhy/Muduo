#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;

/**
 * Channel: 看作通道
 * : 封装sockfd和其感兴趣的event
 * 如: EPOLLIN, EPOLLOUT事件
 * 同时绑定了poller返回的具体事件
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;              // 事件回调
    using ReadEventCallback = std::function<void(Timestamp)>; // 只读事件的回调

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后, 处理事件(调用相应的回调)
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当Channel被手动remove掉, Channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }

    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }

    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll()
    {
        events_ = KNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == KNoneEvent; } // 记录当前Channel是否注册感兴趣的事件
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // One loop per thread
    // 一个线程有一个EventLoop, 一个EventLoop里有一个Poller, 一个Poller上可以监听很多的Channel
    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime); // 受保护的处理事件

    static const int KNoneEvent;  // 目前不对任何事情感兴趣
    static const int kReadEvent;  // 对读事件感兴趣
    static const int kWriteEvent; // 对写事件感兴趣

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    /**
     * 因为Channel通道里面能够获知fd最终发生的具体的事件,
     * 所以它负责调用具体事件的回调操作
     */
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
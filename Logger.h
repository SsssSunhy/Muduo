#pragma once

#include <string>

#include "noncopyable.h"

// 定义宏, 便于用下方方式操作
// ##__VA_ARGS__前面加上##的作用是: 当可变参数的个数为0时, 这里的##可以把前面多余的","去掉, 否则会编译出错
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logMsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logMsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logMsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logMsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logMsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logMsgFormat, ...)
#endif

/**
 * 定义日志级别:
 * INFO: 打印重要的流程信息
 * ERROR: 一些错误, 但是不影响继续执行
 * FATAL: 系统无法向下运行了, 必须输出关键信息
 * DEBUG: 信息很多, 默认都是关闭的
 */
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出日志类
class Logger : noncopyable
{
public:
    // 获取日志唯一的实例对象
    static Logger &instance();

    // 设置日志级别
    void setLogLevel(int Level);

    // 写日志
    void log(std::string msg);

private:
    int logLevel_; // 日志等级
    Logger() {}
};
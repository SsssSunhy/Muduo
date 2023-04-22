#include <time.h>

#include "Timestamp.h"

// 默认Timestamp构造函数
Timestamp::Timestamp()
    : microSecondsSinceEpoch_(0)
{
}

// 带参数的Timestamp构造函数
Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

// 获取当前时间
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}

// 控制格式
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);  // 调用localtime返回时间
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}

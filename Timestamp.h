#pragma once

#include <iostream>

class Timestamp
{
public:
    Timestamp();    // 默认Timestamp构造函数
    explicit Timestamp(int64_t microSecondsSinceEpoch);  // 带参数的Timestamp构造函数
    static Timestamp now(); // 获取当前时间
    std::string toString() const;   // 控制格式
private:
    int64_t microSecondsSinceEpoch_;    // 表示时间
};
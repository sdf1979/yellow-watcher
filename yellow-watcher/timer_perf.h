﻿// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <chrono>
#include <mutex>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "encoding_string.h"

using clock_ = std::chrono::high_resolution_clock;
using second_ = std::chrono::duration<double, std::ratio<1>>;

#define __FILENAME__ (strrchr(__FILE__, '\\')?strrchr(__FILE__, '\\')+1:__FILE__)
#define MEASUREMENT Measurement measurement(__FILENAME__, __func__) 

class TimerPerf {
private:
    static TimerPerf* instance_;
    static std::mutex mutex_;
    std::unordered_map<std::string, std::pair<std::chrono::nanoseconds, std::uint64_t>> measurements_;
protected:
    TimerPerf() = default;
public:
    TimerPerf(TimerPerf&) = delete;
    void Print();
    void operator=(const TimerPerf&) = delete;
    static TimerPerf* GetInstance();
    void Elapsed(const std::string& id, std::chrono::time_point<clock_> start);
    void Clear() { measurements_.clear(); }
};

class Measurement {
private:
    TimerPerf* timer_perf_;
    std::chrono::time_point<clock_> start_;
    std::string id_;
public:
    Measurement(std::string file, std::string func);
    ~Measurement();
};

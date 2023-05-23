#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>

using time_point = std::chrono::time_point<std::chrono::system_clock>;

std::tm ToTm(time_point tp);
std::uint64_t ToUint64(std::tm tm, std::uint32_t mseconds = 0);
std::uint64_t ToUint64(int year = 0, int mon = 0, int day = 0, int hour = 0, int min = 0, int sec = 0, std::uint32_t mseconds = 0);
time_point NowMinute();
std::string ToString(uint64_t date);
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>
#include "functions.h"

using time_point = std::chrono::time_point<std::chrono::system_clock>;

namespace TechLogOneC {
	std::tm ToTm(time_point tp);
	std::uint64_t ToUint64(std::tm tm, std::uint32_t mseconds = 0);
	std::uint64_t ToUint64(int year = 0, int mon = 0, int day = 0, int hour = 0, int min = 0, int sec = 0, std::uint32_t mseconds = 0);
	time_point NowMinute();
	std::tm GetTime();
	std::uint64_t RoundMinute(std::uint64_t date);
	time_point RoundMinute(std::tm tm);
	std::uint64_t Microseconds(std::uint64_t date);
	std::string ToDateFormatString(std::uint64_t date, std::stringstream& ss);
	template<typename T>
	std::string ToDateFormatString(T t, std::stringstream& ss) {
		return UnsupportedDataType(t);
	}
}
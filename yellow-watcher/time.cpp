#include "time.h"

std::stringstream ss;

std::tm ToTm(time_point tp) {
	std::time_t tt = std::chrono::system_clock::to_time_t(tp);
	std::tm tm;
	localtime_s(&tm, &tt);
	return tm;
}

std::uint64_t ToUint64(std::tm tm, std::uint32_t mseconds) {
	return
		((tm.tm_year + 1900) % 100) * static_cast<uint64_t>(10000000000000000) +
		(tm.tm_mon + 1) * static_cast<uint64_t>(100000000000000) +
		tm.tm_mday * static_cast<uint64_t>(1000000000000) +
		tm.tm_hour * static_cast<uint64_t>(10000000000) +
		tm.tm_min * static_cast<uint64_t>(100000000) +
		tm.tm_sec * static_cast<uint64_t>(1000000) +
		mseconds;
}

std::uint64_t ToUint64(int year, int mon, int day, int hour, int min, int sec, std::uint32_t mseconds) {
	return
		(year % 100) * static_cast<uint64_t>(10000000000000000) +
		mon * static_cast<uint64_t>(100000000000000) +
		day * static_cast<uint64_t>(1000000000000) +
		hour * static_cast<uint64_t>(10000000000) +
		min * static_cast<uint64_t>(100000000) +
		sec * static_cast<uint64_t>(1000000) +
		mseconds;
}

time_point NowMinute() {
	std::tm tm = ToTm(std::chrono::system_clock::now());
	tm.tm_sec = 0;
	return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string ToString(uint64_t date) {
	int year = date / static_cast <uint64_t>(10000000000000000);
	date = date - year * static_cast <uint64_t>(10000000000000000);
	int month = date / static_cast <uint64_t>(100000000000000);
	date = date - month * static_cast <uint64_t>(100000000000000);
	int day = date / static_cast <uint64_t>(1000000000000);
	date = date - day * static_cast <uint64_t>(1000000000000);
	int hour = date / static_cast <uint64_t>(10000000000);
	date = date - hour * static_cast <uint64_t>(10000000000);
	int minute = date / static_cast <uint64_t>(100000000);
	date = date - minute * static_cast <uint64_t>(100000000);
	int second = date / static_cast <uint64_t>(1000000);

	ss << std::dec << "20" << std::setw(2) << std::setfill('0') << year
		<< "-" << std::setw(2) << std::setfill('0') << month
		<< "-" << std::setw(2) << std::setfill('0') << day
		<< "T" << std::setw(2) << std::setfill('0') << hour
		<< ":" << std::setw(2) << std::setfill('0') << minute
		<< ":" << std::setw(2) << std::setfill('0') << second;
	std::string date_str = ss.str();
	ss.clear();
	ss.str("");
	return date_str;
}
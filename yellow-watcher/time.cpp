#include "time.h"

using namespace std;

namespace TechLogOneC {

	tm ToTm(time_point tp) {
		time_t tt = chrono::system_clock::to_time_t(tp);
		tm tm;
		localtime_s(&tm, &tt);
		return tm;
	}

	uint64_t ToUint64(tm tm, uint32_t mseconds) {
		return
			((tm.tm_year + 1900) % 100) * static_cast<uint64_t>(10000000000000000) +
			(tm.tm_mon + 1) * static_cast<uint64_t>(100000000000000) +
			tm.tm_mday * static_cast<uint64_t>(1000000000000) +
			tm.tm_hour * static_cast<uint64_t>(10000000000) +
			tm.tm_min * static_cast<uint64_t>(100000000) +
			tm.tm_sec * static_cast<uint64_t>(1000000) +
			mseconds;
	}

	uint64_t ToUint64(int year, int mon, int day, int hour, int min, int sec, uint32_t mseconds) {
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
		tm tm = ToTm(chrono::system_clock::now());
		tm.tm_sec = 0;
		return chrono::system_clock::from_time_t(std::mktime(&tm));
	}	

	tm GetTime() {
		return ToTm(chrono::system_clock::now());
	}

	uint64_t RoundMinute(uint64_t date) {
		return (date / 100000000) * 100000000;
	}

	time_point RoundMinute(std::tm tm) {
		tm.tm_sec = 0;
		return chrono::system_clock::from_time_t(std::mktime(&tm));
	}

	std::uint64_t Microseconds(uint64_t date) {
		return date % 1000000;
	}

	string ToDateFormatString(uint64_t date, stringstream& ss) {
		uint64_t year = date / static_cast <uint64_t>(10000000000000000);
		date = date - year * static_cast <uint64_t>(10000000000000000);
		uint64_t month = date / static_cast <uint64_t>(100000000000000);
		date = date - month * static_cast <uint64_t>(100000000000000);
		uint64_t day = date / static_cast <uint64_t>(1000000000000);
		date = date - day * static_cast <uint64_t>(1000000000000);
		uint64_t hour = date / static_cast <uint64_t>(10000000000);
		date = date - hour * static_cast <uint64_t>(10000000000);
		uint64_t minute = date / static_cast <uint64_t>(100000000);
		date = date - minute * static_cast <uint64_t>(100000000);
		uint64_t second = date / static_cast <uint64_t>(1000000);
		uint64_t msecond = date % static_cast <uint64_t>(1000000);

		ss << std::dec << "20" << std::setw(2) << std::setfill('0') << year
			<< "-" << std::setw(2) << std::setfill('0') << month
			<< "-" << std::setw(2) << std::setfill('0') << day
			<< "T" << std::setw(2) << std::setfill('0') << hour
			<< ":" << std::setw(2) << std::setfill('0') << minute
			<< ":" << std::setw(2) << std::setfill('0') << second;
		if (msecond) {
			ss << "." << std::setw(6) << std::setfill('0') << msecond;
		}
		string date_str = ss.str();
		ss.clear();
		ss.str("");
		return date_str;
	}
}
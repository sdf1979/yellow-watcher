#pragma once

#include <string>
#include <sstream>
#include "boost/json.hpp"
#include "time.h"
#include "functions.h"

namespace TechLogOneC {

	struct EventOptions {
		std::uint64_t count_ = 0;
		std::uint64_t duration_ = 0;
		std::uint64_t rows_ = 0;
		std::uint64_t rows_affected_ = 0;
		std::uint64_t rows_processed_ = 0;
		double estimate_rows_processed_ = 0.0;
		double data_size_ = 0.0;
		double estimate_data_size_ = 0.0;
		boost::json::object AsJsonObject() const;
		boost::json::array AsJsonArray() const;
		boost::json::array ValueNames() const;
	};
	EventOptions& operator+=(EventOptions& lhs, const EventOptions& rhs);

	struct ManagedLockEvent {
		std::uint64_t time_;
		std::string type_;
		std::string data_base_;
		std::string computer_;
		std::string session_id_;
		std::string wait_connections_;
		std::string user_;
		std::string last_string_context_;
		std::string context_;
		ManagedLockEvent(std::uint64_t time, std::string type, std::string data_base, std::string computer, std::string session_id,
			std::string wait_connections, std::string user_, std::string last_string_context, std::string context_);
		static boost::json::array Columns();
		boost::json::array ToJsonArray(std::stringstream& ss);
	};

	struct Measurment {
		std::uint64_t value_ = 0;
		std::uint64_t count_ = 0;
		boost::json::object AsJsonObject() const;
		boost::json::array AsJsonArray() const;
		boost::json::array ValueNames() const;
	};
	Measurment& operator+=(Measurment& lhs, const Measurment& rhs);
}

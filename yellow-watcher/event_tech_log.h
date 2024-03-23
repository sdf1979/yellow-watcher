#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "boost/json.hpp"
#include "time.h"
#include "encoding_string.h"
#include "SqlTextHash.h"

using sql_plan_token = std::tuple<std::uint64_t, std::uint64_t, double, double, double, std::uint64_t, double, double, std::string>;

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

	struct LongRequestEvent {
		std::uint64_t time_ = 0;
		std::uint64_t duration_ = 0;
		std::uint64_t dbpid_ = 0;
		std::uint64_t rows_ = 0;
		std::uint64_t rows_affected_ = 0;
		std::uint64_t rows_processed_ = 0;
		double estimate_rows_processed_ = 0.0;
		double data_size_ = 0.0;
		double estimate_data_size_ = 0.0;
		std::string data_base_;
		std::string computer_;
		std::string session_id_;
		std::string user_;
		std::string last_string_context_;
		std::string context_;
		std::string sql_text_;
		std::string sql_text_hash_;
		std::string sql_plan_text_;
		std::vector<sql_plan_token> sql_plan_tokens_;
		static boost::json::array Columns();
		boost::json::array ToJsonArray(std::stringstream& ss);
	};

	struct MsSqlExcpEvent {
		std::uint64_t time_ = 0;
		std::string computer_;
		std::string session_id_;
		std::string user_;
		std::string data_base_;
		std::uint64_t dbpid_ = 0;
		std::string last_string_context_;
		std::string context_;
		std::string descr_;
		std::string type_;
		static boost::json::array Columns();
		boost::json::array ToJsonArray(std::stringstream& ss);
		static std::string TypeFromDescr(const std::string* descr);
	};
}

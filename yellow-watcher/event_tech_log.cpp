// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "event_tech_log.h"

using namespace std;

namespace TechLogOneC {

	boost::json::object EventOptions::AsJsonObject() const {
		namespace json = boost::json;
		json::object j_object;
		j_object.emplace("count", count_);
		j_object.emplace("sum_dur", duration_);
		j_object.emplace("avg_dur", count_ ? duration_ / count_ : 0);
		j_object.emplace("rows_returned", rows_);
		j_object.emplace("avg_rows_returned", std::floor((count_ ? (1.0 * rows_) / count_ : 0.0) * 1000) / 1000);
		j_object.emplace("rows_affected", rows_affected_);
		j_object.emplace("avg_rows_affected", std::floor((count_ ? (1.0 * rows_affected_) / count_ : 0.0) * 1000) / 1000);
		j_object.emplace("rows_processed", rows_processed_);
		j_object.emplace("estimate_rows_processed", std::floor(estimate_rows_processed_ * 1000) / 1000);
		j_object.emplace("delta_rows_processed", std::floor(fabs(rows_processed_ - estimate_rows_processed_) * 1000) / 1000);
		j_object.emplace("avg_rows_processed", std::floor((count_ ? (1.0 * rows_processed_) / count_ : 0.0) * 1000) / 1000);
		j_object.emplace("avg_estimate_rows_processed", std::floor((count_ ? estimate_rows_processed_ / count_ : 0.0) * 1000) / 1000);
		j_object.emplace("data_size", std::floor(data_size_ * 1000) / 1000);
		j_object.emplace("estimate_data_size", std::floor(estimate_data_size_ * 1000) / 1000);
		j_object.emplace("avg_data_size", std::floor((count_ ? data_size_ / count_ : 0.0) * 1000) / 1000);
		j_object.emplace("avg_estimate_data_size", std::floor((count_ ? estimate_data_size_ / count_ : 0.0) * 1000) / 1000);
		return j_object;
	}

	boost::json::array EventOptions::AsJsonArray() const {
		namespace json = boost::json;
		json::array j_array;
		j_array.emplace_back(count_);
		j_array.emplace_back(duration_);
		j_array.emplace_back(rows_);
		j_array.emplace_back(rows_affected_);
		j_array.emplace_back(rows_processed_);
		j_array.emplace_back(std::floor(estimate_rows_processed_ * 1000) / 1000);
		j_array.emplace_back(std::floor(data_size_ * 1000) / 1000);
		j_array.emplace_back(std::floor(estimate_data_size_ * 1000) / 1000);
		return j_array;
	}

	boost::json::array EventOptions::ValueNames() const {
		return { "count" , "sum_dur", "rows_returned", "rows_affected", "rows_processed", "estimate_rows_processed", "data_size", "estimate_data_size" };
	}

	EventOptions& operator+=(EventOptions& lhs, const EventOptions& rhs) {
		lhs.count_ += rhs.count_;
		lhs.duration_ += rhs.duration_;
		lhs.rows_ += rhs.rows_;
		lhs.rows_affected_ += rhs.rows_affected_;
		lhs.rows_processed_ += rhs.rows_processed_;
		lhs.estimate_rows_processed_ += rhs.estimate_rows_processed_;
		lhs.data_size_ += rhs.data_size_;
		lhs.estimate_data_size_ += rhs.estimate_data_size_;
		return lhs;
	}

	ManagedLockEvent::ManagedLockEvent(uint64_t time, string type, string data_base, string computer, string session_id,
		string wait_connections, string user, string last_string_context, string context) :
		time_(time),
		type_(type),
		data_base_(data_base),
		computer_(computer),
		session_id_(session_id),
		wait_connections_(wait_connections),
		user_(user),
		last_string_context_(last_string_context),
		context_(context) {}

	boost::json::array ManagedLockEvent::Columns() {
		return { "date", "microseconds", "type", "data_base", "computer", "session", "wait_connections", "user",
			"last_string_context", "hash_last_string_context", "context", "hash_context"};
	}

	boost::json::array ManagedLockEvent::ToJsonArray(stringstream& ss) {
		return { ToDateFormatString(time_, ss), Microseconds(time_), type_, data_base_, computer_, session_id_,
			wait_connections_, user_, last_string_context_, Soldy::GetSHA256(last_string_context_, ss), context_, Soldy::GetSHA256(context_, ss) };
	}

	boost::json::object Measurment::AsJsonObject() const {
		namespace json = boost::json;
		json::object j_object;
		j_object.emplace("value", value_);
		j_object.emplace("count", count_);
		return j_object;
	}

	boost::json::array Measurment::AsJsonArray() const {
		namespace json = boost::json;
		json::array j_array;
		j_array.emplace_back(value_);
		j_array.emplace_back(count_);
		return j_array;
	}

	boost::json::array Measurment::ValueNames() const {
		return { "value" , "count" };
	}

	Measurment& operator+=(Measurment& lhs, const Measurment& rhs) {
		lhs.value_ += rhs.value_;
		lhs.count_ += rhs.count_;
		return lhs;
	}

	boost::json::array LongRequestEvent::Columns() {
		return { "date", "microseconds", "duration", "dbpid",
			"rows", "rows_affected", "rows_processed", "estimate_rows_processed", "data_size", "estimate_data_size",
			"data_base", "computer", "session", "user",
			"last_string_context", "hash_last_string_context", "context", "hash_context", "sql_text",
			"sql_text_hash", "hash_sql_text_hash", "sql_plan_text", "sql_plan_tokens"};
	}

	boost::json::array LongRequestEvent::ToJsonArray(stringstream& ss) {
		return { ToDateFormatString(time_, ss), Microseconds(time_), duration_, dbpid_,
			rows_, rows_affected_, rows_processed_, estimate_rows_processed_, data_size_, estimate_data_size_,
			data_base_, computer_, session_id_, user_,
			last_string_context_, Soldy::GetSHA256(last_string_context_, ss), context_, Soldy::GetSHA256(context_, ss), sql_text_,
			sql_text_hash_, Soldy::GetSHA256(sql_text_hash_, ss), sql_plan_text_, sql_plan_tokens_ };
	}
	
}
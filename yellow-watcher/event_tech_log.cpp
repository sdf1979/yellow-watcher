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
	
	boost::json::array MsSqlExcpEvent::Columns() {
		return { "date", "microseconds", "computer", "session", "user", "data_base", "dbpid",
			"last_string_context", "hash_last_string_context", "context", "hash_context",
			"descr", "type"};
	}

	boost::json::array MsSqlExcpEvent::ToJsonArray(stringstream& ss) {
		return { ToDateFormatString(time_, ss), Microseconds(time_), computer_, session_id_, user_, data_base_, dbpid_,
			last_string_context_, Soldy::GetSHA256(last_string_context_, ss), context_, Soldy::GetSHA256(context_, ss),
			descr_, type_};
	}

	string MsSqlExcpEvent::TypeFromDescr(const std::string* descr) {
		if (descr->find(u8"Lock request time out period exceeded") != string::npos) return u8"Таймаут ожидания блокировки";
		if (descr->find(u8"Превышено время ожидания запроса на блокировку") != string::npos) return u8"Таймаут ожидания блокировки";
		if (descr->find(u8"the session is in the kill state") != string::npos) return u8"Принудительное завершение сеанса";
		if (descr->find(u8"The semaphore timeout period has expired") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"Превышен таймаут семафора") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"An existing connection was forcibly closed by the remote host") != string::npos) return u8"Удаленный хост закрыл соединение";
		if (descr->find(u8"Удаленный хост принудительно разорвал существующее подключение") != string::npos) return u8"Удаленный хост закрыл соединение";
		if (descr->find(u8"Ошибка связи") != string::npos) return u8"Удаленный хост закрыл соединение";
		if (descr->find(u8"Не удается завершить вход в систему") != string::npos) return u8"Удаленный хост закрыл соединение";
		if (descr->find(u8"deadlocked on lock resources with another process") != string::npos) return u8"Взаимоблокировка";
		if (descr->find(u8"Unable to access availability database") != string::npos) return u8"Ошибка доступа к реплике";
		if (descr->find(u8"Не найдена база данных") != string::npos) return u8"Не найдена база";
		if (descr->find(u8"Cannot open database") != string::npos && descr->find("The login failed") != string::npos) return u8"Ошибка аутентификации";
		if (descr->find(u8"Поставщик именованных каналов") != string::npos) return u8"Ошибка именованных каналов";
		if (descr->find(u8"Cannot insert duplicate key") != string::npos) return u8"Нарушено условие уникальности данных";
		if (descr->find(u8"The specified network name is no longer available") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"Указанное сетевое имя более недоступно") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"A connection attempt failed") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"Попытка установить соединение была безуспешной") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"Timeout error") != string::npos) return u8"Ошибка TCP";
		if (descr->find(u8"User does not have permission to use the KILL statement") != string::npos) return u8"Нет разрешения KILL";
		return u8"Прочее";
	}
}
#include "ttimeout_aggregator.h"

using namespace std;

string_view LastStringContext(const string& str) {
	string_view sv(str);
	auto pos = str.rfind('\n');
	if (pos != str.npos) {
		sv.remove_prefix(pos + 1);
	}
	pos = sv.find_first_not_of('\t');
	if (pos != sv.npos) {
		sv.remove_prefix(pos);
	}
	if (sv.back() == '\'') {
		sv.remove_suffix(1);
	}
	return sv;
}

string ComputeSHA256(stringstream& ss, const string& str) {
	string hash_string;
	EVP_MD_CTX* context = EVP_MD_CTX_new();
	if (context) {
		if (EVP_DigestInit_ex(context, EVP_sha256(), NULL)) {
			if (EVP_DigestUpdate(context, str.c_str(), str.length())) {
				unsigned char hash[EVP_MAX_MD_SIZE];
				unsigned int lengthOfHash = 0;
				if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
					for (unsigned int i = 0; i < lengthOfHash; ++i)	{
						ss << hex << setw(2) << setfill('0') << (int)hash[i];
					}
					hash_string = ss.str();
					ss.clear();
					ss.str("");
				}
			}
		}
		EVP_MD_CTX_free(context);
	}
	return hash_string;
}

void ContextAgregator::Add(const EventData& event_data) {
	++count_;
	auto it = agreagator_.insert({ *event_data.GetContext(), {} });
	++it.first->second;
}

boost::json::object ContextAgregator::ToJsonObject() const {
	namespace json = boost::json;
	json::object j_data;
	stringstream ss;

	json::array j_rows;

	for (auto it = agreagator_.begin(); it != agreagator_.end(); ++it) {
		json::object j_rows_object;
		j_rows_object.emplace("count_ttimeout", it->second);
		j_rows_object.emplace("hash_context", ComputeSHA256(ss, it->first));
		j_rows_object.emplace("context", it->first);
		j_rows.emplace(j_rows.end(), j_rows_object);
	}

	j_data.emplace("rows", j_rows);

	return j_data;
}

void LastStringContextAgregator::Add(const EventData& event_data) {
	++count_;
	string_view last_string_context = LastStringContext(*event_data.GetContext());
	auto it = agreagator_.insert({ string(last_string_context), {} });
	it.first->second.Add(event_data);
}

boost::json::object LastStringContextAgregator::ToJsonObject() const {
	namespace json = boost::json;
	stringstream ss;
	json::object j_data;

	j_data.emplace("count_ttimeout", count_);
	json::array j_rows;

	for (auto it = agreagator_.begin(); it != agreagator_.end(); ++it) {
		json::object j_rows_object;
		j_rows_object.emplace("count_ttimeout", it->second.Count());
		j_rows_object.emplace("hash_laststrcontext", ComputeSHA256(ss, it->first));
		j_rows_object.emplace("laststrcontext", it->first);
		j_rows_object.emplace("rows_context", it->second.ToJsonObject());
		j_rows.emplace(j_rows.end(), j_rows_object);
	}

	j_data.emplace("rows_laststrcontext", j_rows);

	return j_data;
}

void DatabaseContextAgregator::Add(const EventData& event_data) {
	++count_;
	auto it = agreagator_.insert({ *event_data.GetDatabase(), {} });
	it.first->second.Add(event_data);
}

boost::json::object DatabaseContextAgregator::ToJsonObject() const {
	namespace json = boost::json;
	json::object j_data;

	j_data.emplace("count_ttimeout", count_);
	json::array j_rows;

	for (auto it = agreagator_.begin(); it != agreagator_.end(); ++it) {
		json::object j_rows_object;
		j_rows_object.emplace("count_ttimeout", it->second.Count());
		j_rows_object.emplace("database", it->first);
		j_rows_object.emplace("laststrcontext", it->second.ToJsonObject());
		j_rows.emplace(j_rows.end(), j_rows_object);
	}

	j_data.emplace("rows_databases", j_rows);

	return j_data;
}

void TtimeoutAggregator::Add(const tm& time, const EventData& event_data) {
	uint64_t time_event = ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event_data.Minute(), event_data.Second(), event_data.Msecond());
	uint64_t time_minute_event = ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event_data.Minute());
	AddStatistics(time_minute_event, event_data);
	AddEventTtimeout(time_event, event_data);
}

void TtimeoutAggregator::AddStatistics(std::uint64_t time_minute_event, const EventData& event_data) {
	auto it = agreagator_.insert({ time_minute_event, {} });
	it.first->second.Add(event_data);
}

void TtimeoutAggregator::AddEventTtimeout(std::uint64_t time_event, const EventData& event_data) {
	string last_string_context(LastStringContext(*event_data.GetContext()));
	EventTtimeout event_ttimeout = {
		time_event,
		event_data.Name(),
		*event_data.GetComputer(),
		*event_data.GetDatabase(),
		*event_data.GetSession(),
		ComputeSHA256(ss_, last_string_context),
		ComputeSHA256(ss_, *event_data.GetContext()),
		*event_data.GetWaitConnections(),
		*event_data.GetUsr(),
		last_string_context,
		*event_data.GetContext()
	};

	events_ttimeout_.push_back(move(event_ttimeout));
}

void TtimeoutAggregator::Delete(uint64_t minute) {
	agreagator_.extract(minute);
}

boost::json::object JsonInit() {
	namespace json = boost::json;
	json::object j_object;
	j_object.emplace("event_type", "TTIMEOUT");
	
	json::array j_rows;
	j_object.emplace("events_stat", j_rows);

	json::object j_events_ttimeout;
	j_object.emplace("events", j_events_ttimeout);

	return j_object;
}

void JsonAddRowStat(boost::json::array* j_rows, uint64_t time, const DatabaseContextAgregator& db_context_agr) {
	namespace json = boost::json;
	json::object j_row;
	j_row.emplace("date_time", ToString(time));
	j_row.emplace("data", db_context_agr.ToJsonObject());
	j_rows->emplace_back(j_row);
}

void JsonAddRows(vector<EventTtimeout>& events_ttimeout_, boost::json::object* j_events_ttimeout) {
	namespace json = boost::json;

	json::array j_events_ttimeout_columns;
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "time");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "microseconds");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "name");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "computer");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "base");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "session");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "hesh_last_string_context");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "hesh_context");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "wait_connections");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "usr");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "last_string_context");
	j_events_ttimeout_columns.emplace(j_events_ttimeout_columns.end(), "context");
	j_events_ttimeout->emplace("columns", j_events_ttimeout_columns);

	json::array j_events_ttimeout_rows;
	for (auto it = events_ttimeout_.begin(); it != events_ttimeout_.end(); ++it) {
		json::array j_event_ttimeout;
		j_event_ttimeout.emplace(j_event_ttimeout.end(), ToString(it->time_event_));
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->time_event_ % 1000000);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->name_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->computer_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->base_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->session_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->hesh_last_string_context);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->hesh_context_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->wait_connections_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->usr_);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->last_string_context);
		j_event_ttimeout.emplace(j_event_ttimeout.end(), it->context);
		j_events_ttimeout_rows.emplace(j_events_ttimeout_rows.end(), j_event_ttimeout);
	}
	j_events_ttimeout->emplace("rows", j_events_ttimeout_rows);
}

string TtimeoutAggregator::ToJson() {
	namespace json = boost::json;
	json::object j_object = JsonInit();
	
	json::array& j_rows = j_object.find("events_stat")->value().as_array();
	for (auto it = agreagator_.begin(); it != agreagator_.end(); ++it) {
		JsonAddRowStat(&j_rows, it->first, it->second);
	}

	json::object& j_events_ttimeout = j_object.find("events")->value().as_object();
	JsonAddRows(events_ttimeout_, &j_events_ttimeout);

	return json::serialize(j_object);
}

string TtimeoutAggregator::ToJson(uint64_t time) {
	namespace json = boost::json;
	json::object j_object = JsonInit();

	json::array& j_rows = j_object.find("events_stat")->value().as_array();
	auto it = agreagator_.find(time);
	if (it != agreagator_.end()) {
		JsonAddRowStat(&j_rows, it->first, it->second);
	}

	json::object& j_events_ttimeout = j_object.find("events")->value().as_object();
	JsonAddRows(events_ttimeout_, &j_events_ttimeout);

	return json::serialize(j_object);
}
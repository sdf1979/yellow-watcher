#pragma once

#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <vector>
#include "parser.h"
#include "encoding_string.h"
#include "openssl/evp.h"
#include "boost/json.hpp"
#include "time.h"

struct EventTtimeout {
    std::uint64_t time_event_;
    std::string name_;
    std::string computer_;
    std::string base_;
    std::string session_;
    std::string hesh_last_string_context;
    std::string hesh_context_;
    std::string wait_connections_;
    std::string usr_;
    std::string last_string_context;
    std::string context;
};

class ContextAgregator {
    std::uint32_t count_ = 0;
    std::unordered_map<std::string, std::uint32_t> agreagator_;
public:
    std::uint32_t Count() const { return count_; }
    void Add(const EventData& event_data);
    boost::json::object ToJsonObject() const;
};

class LastStringContextAgregator {
    std::uint32_t count_ = 0;
    std::unordered_map<std::string, ContextAgregator> agreagator_;
public:
    std::uint32_t Count() const { return count_; }
    void Add(const EventData& event_data);
    boost::json::object ToJsonObject() const;
};

class DatabaseContextAgregator {
    std::uint32_t count_ = 0;
    std::unordered_map<std::string, LastStringContextAgregator>::iterator it_;
    std::unordered_map<std::string, LastStringContextAgregator> agreagator_;
public:
    std::uint32_t Count() { return count_; }
    void Add(const EventData& event_data);
    boost::json::object ToJsonObject() const;
};

class TtimeoutAggregator {
    std::unordered_map<std::uint64_t, DatabaseContextAgregator> agreagator_;
    std::vector<EventTtimeout> events_ttimeout_;
    void AddStatistics(std::uint64_t minute, const EventData& event_data);
    void AddEventTtimeout(std::uint64_t time_event, const EventData& event_data);
    std::stringstream ss_;
public:
    void Add(const tm& time, const EventData& event_data);
    void Delete(uint64_t minute);
    void ClearEvents() { events_ttimeout_.clear(); }
    std::string ToJson();
    std::string ToJson(uint64_t time);
};



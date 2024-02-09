#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <typeinfo>
#include <type_traits>
#include "openssl/evp.h"
#include "boost/json.hpp"
#include "functions.h"
#include "event_tech_log.h"
#include "time.h"
#include "SqlTextHash.h"

namespace TechLogOneC {

	enum class KeyConversion {
		NoConvertion,
		DateString
	};

	struct GenerationRule {
		KeyConversion convert_key_ = KeyConversion::NoConvertion;
		bool generate_hash_ = false;
	};

	template<typename... Args>
	class EventAggregator;

	template<typename EvOpt, typename T, typename... Args>
	class EventAggregator<EvOpt, T, Args...> {
		std::unordered_map<T, EventAggregator<EvOpt, Args...>> data_;
		std::vector<std::pair<const T*, EventAggregator<EvOpt, Args...>*>> data_sort_;
		EvOpt event_options_;
		GenerationRule generation_rule_;
		boost::json::array ValueNames();
	public:
		void Add(const EvOpt& event_options, const T& t, const Args&... args);
		template<typename... GenerationRules>
		void SetGenerationRule(GenerationRule generation_rule, GenerationRules... generation_rules);
		boost::json::array ToJsonArray();
		boost::json::object ToJsonObject();
		EventAggregator<EvOpt, Args...>* Find(const T& t);
		void Delete(const T& t);
	};

	template<typename EvOpt, typename T>
	class EventAggregator<EvOpt, T> {
		std::unordered_map<T, EvOpt> data_;
		std::vector<std::pair<const T*, EvOpt*>> data_sort_;
		EvOpt event_options_;
		GenerationRule generation_rule_;
	public:
		void Add(const EvOpt& event_options, const T& t);
		void SetGenerationRule(GenerationRule generation_rule);
		boost::json::array ToJsonArray();
	};

	template<typename EvOpt, typename T, typename... Args>
	void EventAggregator<EvOpt, T, Args...>::Add(const EvOpt& event_options, const T& t, const Args&... args) {
		auto it = data_.insert({ t, {} });
		it.first->second.Add(event_options, args...);
	}

	template<typename EvOpt, typename T>
	void EventAggregator<EvOpt, T>::Add(const EvOpt& event_options, const T& t) {
		event_options_ += event_options;
		auto it = data_.insert({ t, {} });
		it.first->second += event_options;
	}

	template<typename EvOpt, typename T, typename... Args> template<typename... GenerationRules>
	void EventAggregator<EvOpt, T, Args...>::SetGenerationRule(GenerationRule generation_rule, GenerationRules... generation_rules) {
		generation_rule_ = generation_rule;
		for (auto it = data_.begin(); it != data_.end(); ++it) {
			it->second.SetGenerationRule(generation_rules...);
		}
	}

	template<typename EvOpt, typename T>
	void EventAggregator<EvOpt, T>::SetGenerationRule(GenerationRule generation_rule) {
		generation_rule_ = generation_rule;
	}

	template<typename EvOpt, typename T, typename... Args>
	boost::json::array EventAggregator<EvOpt, T, Args...>::ValueNames() {
		if (std::is_same<EvOpt, EventOptions>::value) {
			return reinterpret_cast<EventOptions*>(&event_options_)->ValueNames();
		}
		else if (std::is_same<EvOpt, Measurment>::value) {
			return reinterpret_cast<Measurment*>(&event_options_)->ValueNames();
		}
		else if (std::is_same<EvOpt, std::uint64_t>::value) {
			return { "count" };
		}
		else {
			return { UnsupportedDataType(event_options_) };
		}
	}

	template<typename EvOpt, typename T, typename... Args>
	boost::json::object EventAggregator<EvOpt, T, Args...>::ToJsonObject() {
		boost::json::object j_object;
		j_object.emplace("value_names", ValueNames());
		j_object.emplace("data", ToJsonArray());
		return j_object;
	}

	template<typename EvOpt, typename T, typename... Args>
	boost::json::array EventAggregator<EvOpt, T, Args...>::ToJsonArray() {
		namespace json = boost::json;
		std::stringstream ss;
		json::array j_rows;
		for (auto it = data_.begin(); it != data_.end(); ++it) {
			json::object j_row;
			if (generation_rule_.convert_key_ == KeyConversion::DateString) {
				j_row.emplace("key", ToDateFormatString(it->first, ss));
			}
			else {
				j_row.emplace("key", it->first);
			}
			if (generation_rule_.generate_hash_) {
				j_row.emplace("key_hash", Soldy::GetSHA256(it->first, ss));
			}
			j_row.emplace("rows", it->second.ToJsonArray());
			j_rows.emplace_back(j_row);
		}
		return j_rows;
	}

	template<typename EvOpt, typename T>
	boost::json::array EventAggregator<EvOpt, T>::ToJsonArray() {
		namespace json = boost::json;
		std::stringstream ss;
		json::array j_rows;
		for (auto it = data_.begin(); it != data_.end(); ++it) {
			json::object j_row;
			if (generation_rule_.convert_key_ == KeyConversion::DateString) {
				j_row.emplace("key", ToDateFormatString(it->first, ss));
			}
			else {
				j_row.emplace("key", it->first);
			}

			if (generation_rule_.generate_hash_) {
				j_row.emplace("key_hash", Soldy::GetSHA256(it->first, ss));
			}

			if (std::is_same<EvOpt, EventOptions>::value) {
				j_row.emplace("values", reinterpret_cast<EventOptions*>(&it->second)->AsJsonArray());
			}
			else if (std::is_same<EvOpt, Measurment>::value) {
				j_row.emplace("values", reinterpret_cast<Measurment*>(&it->second)->AsJsonArray());
			}
			else if (std::is_same<EvOpt, std::uint64_t>::value) {
				j_row.emplace("values", *reinterpret_cast<std::uint64_t*>(&it->second));
			}
			else {
				j_row.emplace("values", UnsupportedDataType(event_options_));
			}
			j_rows.emplace_back(j_row);
		}
		return j_rows;
	}

	template<typename EvOpt, typename T, typename... Args>
	EventAggregator<EvOpt, Args...>* EventAggregator<EvOpt, T, Args...>::Find(const T& t) {
		auto it = data_.find(t);
		if (it != data_.end()) {
			return &it->second;
		}
		else {
			return nullptr;
		}
	}

	template<typename EvOpt, typename T, typename... Args>
	void EventAggregator<EvOpt, T, Args...>::Delete(const T& t) {
		data_.erase(t);
	}
}
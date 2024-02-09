// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <vector>
#include "openssl/evp.h"
#include "boost/json.hpp"
#include "event_tech_log.h"

namespace TechLogOneC {

	template<typename T>
	class EventAccumulator {
		std::vector<T> accumulator_;
	public:
		void Add(T t);
		void Clear();
		bool Empty();
		boost::json::object ToJsonObject();
	};

	template<typename T>
	void EventAccumulator<T>::Add(T t) {
		accumulator_.push_back(t);
	}

	template<typename T>
	void EventAccumulator<T>::Clear() {
		accumulator_.clear();
	}

	template<typename T>
	bool EventAccumulator<T>::Empty() {
		return accumulator_.empty();
	}

	template<typename T>
	boost::json::object EventAccumulator<T>::ToJsonObject() {
		boost::json::object j_object;
		j_object.emplace("columns", T::Columns());

		boost::json::array j_rows;
		std::stringstream ss;
		for (auto it = accumulator_.begin(); it != accumulator_.end(); ++it) {
			j_rows.emplace_back(it->ToJsonArray(ss));
		}
		j_object.emplace("rows", j_rows);

		return j_object;
	}

}

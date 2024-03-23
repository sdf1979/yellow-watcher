// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "functions.h"

using namespace std;

namespace TechLogOneC {

	string WaitConnectionFromDeadlockConnectionIntersections(const string& str) {
		string_view sv(str);
		auto pos = sv.find(" ");
		if (pos != string_view::npos) {
			sv.remove_prefix(pos + 1);
			pos = sv.find(" ");
			if (pos != string_view::npos) {
				sv.remove_suffix(sv.size() - pos);
				return string(sv);
			}
			else {
				return "";
			}
		}
		else {
			return "";
		}
	}

	string LastStringContext(const string* str) {
		string_view sv(*str);
		if (sv.empty()) return string(sv);
		auto pos = sv.rfind('\n');
		if (pos != sv.npos) {
			sv.remove_prefix(pos + 1);
		}
		pos = sv.find_first_not_of('\t');
		if (pos != sv.npos) {
			sv.remove_prefix(pos);
		}
		if (sv.back() == '\'') {
			sv.remove_suffix(1);
		}
		return string(sv);
	}

	uint64_t RowsToUint64(const string* str) {
		if (str->empty() || *str == "-1") {
			return 0;
		}
		else {
			return stoull(*str);
		}
	}

	uint64_t GetInt64FromPlanSqlText(string_view& str) {
		auto pos = str.find(',');
		uint64_t res = std::stoull(string(str.substr(0, pos)));
		str.remove_prefix(pos + 1);
		return res;
	}

	double GetDoubleFromPlanSqlText(string_view& str) {
		auto pos = str.find(',');
		double res = stod(string(str.substr(0, pos)));
		str.remove_prefix(pos + 1);
		return res;
	}

	string GetStmtFromPlanSqlText(string_view& str) {
		auto pos = str.find("|--");
		str.remove_prefix(pos + 3);
		return string(str);
	}

	string GetMethodFromPlanSqlText(string_view& str) {
		auto pos = str.find('(');
		return string(str.substr(0, pos));
	}

	uint64_t GetDbPid(const string* str) {
		if (!str->empty() && (*str)[0] >= '0' && (*str)[0] <= '9') {
			return std::stoull(*str);
		}
		else {
			return 0;
		}
	}
}
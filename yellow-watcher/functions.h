#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <typeinfo>
#include "openssl/evp.h"

namespace TechLogOneC {
	std::string GetSHA256(const std::string& str, std::stringstream& ss);
	template<typename T>
	std::string GetSHA256(const T& t, std::stringstream& ss) {
		std::string msg = "Unsupported data type: ";
		msg.append(typeid(t).name());
		return msg;
	}

	std::string WaitConnectionFromDeadlockConnectionIntersections(const std::string& str);
	std::string LastStringContext(const std::string* str);
	void DeleteSingleQuotesLeftRight(std::string_view& sv);
	void DeleteDoubleQuotesLeftRight(std::string_view& sv);
	void DeleteParam(std::string_view& sv);
	void DeleteSpExecuteSql(std::string_view& sv);
	void ChangeCreateClusteredIndex(std::string& sql_hash, std::string_view& sv);
	std::uint64_t RowsToUint64(const std::string* str);
	std::string SqlHashDbMsSql(const std::string* str);
	std::uint64_t GetInt64FromPlanSqlText(std::string_view& str);
	double GetDoubleFromPlanSqlText(std::string_view& str);
	std::string GetStmtFromPlanSqlText(std::string_view& str);
	std::string GetMethodFromPlanSqlText(std::string_view& str);

	template<typename T>
	std::string UnsupportedDataType(T t) {
		std::string msg = "Unsupported data type: ";
		msg.append(typeid(t).name());
		return msg;
	}
}

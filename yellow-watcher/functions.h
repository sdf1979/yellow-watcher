#pragma once

#include <string>

namespace TechLogOneC {
	std::string WaitConnectionFromDeadlockConnectionIntersections(const std::string& str);
	std::string LastStringContext(const std::string* str);
	std::uint64_t RowsToUint64(const std::string* str);
	std::uint64_t GetInt64FromPlanSqlText(std::string_view& str);
	double GetDoubleFromPlanSqlText(std::string_view& str);
	std::string GetStmtFromPlanSqlText(std::string_view& str);
	std::string GetMethodFromPlanSqlText(std::string_view& str);
	std::uint64_t GetDbPid(const std::string* str);

	template<typename T>
	std::string UnsupportedDataType(T t) {
		std::string msg = "Unsupported data type: ";
		msg.append(typeid(t).name());
		return msg;
	}
}

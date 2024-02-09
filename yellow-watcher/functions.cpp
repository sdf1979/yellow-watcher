// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "functions.h"

using namespace std;

namespace TechLogOneC {
//
//	string GetSHA256(const string& str, stringstream& ss) {
//		string hash_string;
//		EVP_MD_CTX* context = EVP_MD_CTX_new();
//		if (context) {
//			if (EVP_DigestInit_ex(context, EVP_sha256(), NULL)) {
//				if (EVP_DigestUpdate(context, str.c_str(), str.length())) {
//					unsigned char hash[EVP_MAX_MD_SIZE];
//					unsigned int lengthOfHash = 0;
//					if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
//						for (unsigned int i = 0; i < lengthOfHash; ++i) {
//							ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
//						}
//						hash_string = ss.str();
//						ss.clear();
//						ss.str("");
//					}
//				}
//			}
//			EVP_MD_CTX_free(context);
//		}
//		return hash_string;
//	}
//
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

	string LastStringContext(const std::string* str) {
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
//
//	void DeleteSingleQuotesLeftRight(string_view& sv) {
//		if (sv.front() == '\'' and sv.back() == '\'') {
//			sv.remove_prefix(1);
//			sv.remove_suffix(1);
//		}
//	}
//
//	void DeleteDoubleQuotesLeftRight(string_view& sv) {
//		if (sv.front() == '"' and sv.back() == '"') {
//			sv.remove_prefix(1);
//			sv.remove_suffix(1);
//		}
//	}
//
//	void DeleteParam(string_view& sv) {
//		auto pos = sv.rfind("\np_0:");
//		if (pos != string_view::npos) {
//			sv.remove_suffix(sv.size() - pos);
//		}
//	}
//
//	void DeleteSpExecuteSql(string_view& sv) {
//		if (sv.substr(0, 22) == "{call sp_executesql(N'") {
//			sv.remove_prefix(22);
//			auto pos = sv.find("',");
//			if (pos != string_view::npos) {
//				sv.remove_suffix(sv.size() - pos);
//			}
//		}
//	}
//
//	void ChangeCreateClusteredIndex(string& sql_hash, string_view& sv) {
//		const static int SIZE_CREATER_CLUSTERED_INDEX = 26;
//		if (sv.size() >= SIZE_CREATER_CLUSTERED_INDEX && sv.substr(0, 26) == "CREATE CLUSTERED INDEX idx") {
//			auto pos = sv.find(' ', 26);
//			if (pos != string_view::npos) {
//				sv.remove_prefix(pos + 1);
//				sql_hash.append("CREATECLUSTEREDINDEXidx");
//			}
//		}
//	}
//
	uint64_t RowsToUint64(const string* str) {
		if (str->empty() || *str == "-1") {
			return 0;
		}
		else {
			return stoull(*str);
		}
	}
//
//	string SqlHashDbMsSql(const string* str) {
//		string_view sv(*str);
//		DeleteSingleQuotesLeftRight(sv);
//		DeleteDoubleQuotesLeftRight(sv);
//		DeleteParam(sv);
//		DeleteSpExecuteSql(sv);
//		string sql_hash;
//		sql_hash.reserve(sv.size());
//		ChangeCreateClusteredIndex(sql_hash, sv);
//		string stack;
//		for (auto it = sv.begin(); it != sv.end(); ++it) {
//			switch (*it) {
//			case ' ':
//				if (stack == " #tt") {
//					sql_hash.append("#tt");
//					stack.clear();
//				}
//				else if (stack == " IN") {
//					stack.push_back(*it);
//				}
//				else if (stack == " IN (?") {
//					continue;
//				}
//				else {
//					stack.push_back(*it);
//				}
//				continue;
//			case '\'':
//				if (stack == "..#tt") {
//					sql_hash.append("#tt");
//					stack.clear();
//				}
//				break;
//			case '.':
//				stack.push_back(*it);
//				break;
//			case '\r':
//				continue;
//			case '\n':
//				continue;
//			case '#':
//				if (stack == " " || stack == "..") stack.push_back(*it);
//				continue;
//			case 't':
//				if (stack == " #" || stack == " #t" || stack == "..#" || stack == "..#t") {
//					stack.push_back(*it);
//					continue;
//				}
//				else {
//					break;
//				}
//			case '?':
//				if (stack == " IN (") {
//					stack.push_back(*it);
//				}
//				else if (stack == " IN (?") {
//					continue;
//				}
//				sql_hash.push_back('P');
//				continue;
//			case '\"':
//				continue;
//			case 'I':
//				if (stack == " ") {
//					stack.push_back(*it);
//				}
//				break;
//			case 'N':
//				if (stack == " I") {
//					stack.push_back(*it);
//				}
//				break;
//			case '(':
//				if (stack == " IN ") {
//					stack.push_back(*it);
//				}
//				break;
//			case ')':
//				if (stack == " IN (?") {
//					sql_hash.append(",...,P)");
//					stack.clear();
//					continue;
//				}
//				break;
//			default:
//				if (stack == " #tt" || stack == "..#tt" || stack == " IN (?") continue;
//				if (!stack.empty()) stack.clear();
//				break;
//			}
//			sql_hash.push_back(*it);
//		}
//		return sql_hash;
//	}
//
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

	string GetStmtFromPlanSqlText(std::string_view& str) {
		auto pos = str.find("|--");
		str.remove_prefix(pos + 3);
		return string(str);
	}

	string GetMethodFromPlanSqlText(std::string_view& str) {
		auto pos = str.find('(');
		return string(str.substr(0, pos));
	}
}
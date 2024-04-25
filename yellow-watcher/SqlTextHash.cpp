#include "SqlTextHash.h"

using namespace std;

namespace Soldy {

	const string FIND_Q = "_Q_";
	const string FIND_F = "_F_";

	string SqlHashDbMsSql(const string* str) {
		string_view sv(*str);
		DeleteSingleQuotesLeftRight(sv);
		DeleteDoubleQuotesLeftRight(sv);
		DeleteLeftParameters(sv);
		DeleteParam(sv);
		DeleteSpExecuteSql(sv);
		DeleteXmlTag(sv);
		string replacing_string_tab;
		sv = ReplacingTab(sv, replacing_string_tab);
		string replacing_string;
		sv = ReplacingParameters(sv, replacing_string);
		string sql_hash;
		sql_hash.reserve(sv.size());
		ChangeCreateClusteredIndex(sql_hash, sv);
		string stack;
		for (auto it = sv.begin(); it != sv.end(); ++it) {
			switch (*it) {
			case ' ':
				if (stack == " #tt") {
					sql_hash.append("#tt");
					stack.clear();
				}
				else if (stack == " IN") {
					stack.push_back(*it);
				}
				else if (stack == " IN (?") {
					continue;
				}
				else {
					stack.push_back(*it);
				}
				continue;
			case '\'':
				if (stack == "..#tt") {
					sql_hash.append("#tt");
					stack.clear();
				}
				break;
			case '.':
				stack.push_back(*it);
				break;
			case '\r':
				continue;
			case '\n':
				continue;
			case '#':
				if (stack == " " || stack == "..") stack.push_back(*it);
				continue;
			case 't':
				if (stack == " #" || stack == " #t" || stack == "..#" || stack == "..#t") {
					stack.push_back(*it);
					continue;
				}
				else {
					break;
				}
			case '?':
				if (stack == " IN (") {
					stack.push_back(*it);
				}
				else if (stack == " IN (?") {
					continue;
				}
				sql_hash.push_back('P');
				continue;
			case '\"':
				continue;
			case 'I':
				if (stack == " ") {
					stack.push_back(*it);
				}
				break;
			case 'N':
				if (stack == " I") {
					stack.push_back(*it);
				}
				break;
			case '(':
				if (stack == " IN ") {
					stack.push_back(*it);
				}
				break;
			case ')':
				if (stack == " IN (?") {
					sql_hash.append(",...,P)");
					stack.clear();
					continue;
				}
				break;
			default:
				if (stack == " #tt" || stack == "..#tt" || stack == " IN (?") continue;
				if (!stack.empty()) stack.clear();
				break;
			}
			sql_hash.push_back(*it);
		}
		ReplaceColumnName(&sql_hash);
		return sql_hash;
	}

	void DeleteLeftParameters(std::string_view& sv) {
		if (sv.substr(0, 2) == "(@") {
			int stack = 0;
			for (size_t index = 0; index < sv.size(); ++index) {
				if (sv[index] == '(') {
					++stack;
				}
				else if(sv[index] == ')') {
					--stack;
				}
				if (!stack) {
					sv.remove_prefix(index + 1);
					break;
				}
			}
		}
	}

	void DeleteSingleQuotesLeftRight(string_view& sv) {
		if (sv.front() == '\'' && sv.back() == '\'') {
			sv.remove_prefix(1);
			sv.remove_suffix(1);
		}
	}

	void DeleteDoubleQuotesLeftRight(string_view& sv) {
		if (sv.front() == '"' && sv.back() == '"') {
			sv.remove_prefix(1);
			sv.remove_suffix(1);
		}
	}

	void DeleteParam(string_view& sv) {
		auto pos = sv.rfind("\np_0:");
		if (pos != string_view::npos) {
			sv.remove_suffix(sv.size() - pos);
		}
	}

	void DeleteSpExecuteSql(string_view& sv) {
		if (sv.substr(0, 22) == "{call sp_executesql(N'") {
			sv.remove_prefix(22);
			auto pos = sv.find("',");
			if (pos != string_view::npos) {
				sv.remove_suffix(sv.size() - pos);
			}
		}
	}

	void DeleteXmlTag(string_view& sv) {
		if (sv.substr(0, 10) == "<?query --" && sv.substr(sv.size() - 4, 4) == "--?>") {
			sv.remove_prefix(10);
			sv.remove_suffix(4);
		}
	}

	string_view ReplacingTab(string_view sv, string& str) {
		for (;;) {
			auto pos = sv.find('\t');
			if (pos != string_view::npos) {
				str.append(sv.substr(0, pos)).append(" ");
				sv.remove_prefix(pos + 1);
			}
			else {
				break;
			}
		}
		if (str.size()) {
			str.append(sv);
			return string_view(str);
		}
		else {
			return sv;
		}
	}

	void ChangeCreateClusteredIndex(string& sql_hash, string_view& sv) {
		const static int SIZE_CREATER_CLUSTERED_INDEX = 26;
		if (sv.size() >= SIZE_CREATER_CLUSTERED_INDEX && sv.substr(0, 26) == "CREATE CLUSTERED INDEX idx") {
			auto pos = sv.find(' ', 26);
			if (pos != string_view::npos) {
				sv.remove_prefix(pos + 1);
				sql_hash.append("CREATECLUSTEREDINDEXidx");
			}
		}
	}

	void ReplaceColumnName(string* str) {
		size_t posQ = 0;
		size_t posF;
		for (;;) {
			posQ = str->find(FIND_Q, posQ);
			if (posQ != string::npos) {
				posQ += FIND_Q.length();
				posF = str->find(FIND_F, posQ);
				if (posF != string::npos) {
					if (posF - posQ == 3) {
						if (
							(*str)[posQ + 2] > '0' && (*str)[posQ + 2] <= '9'
							&& (*str)[posQ + 1] >= '0' && (*str)[posQ + 1] <= '9'
							&& (*str)[posQ] >= '0' && (*str)[posQ] <= '9'
							) {
							memset(str->data() + posQ, '0', 3);
						}
					}
					posQ = posF + FIND_F.length();
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}
	}

	string_view ReplacingParameters(string_view sv, string& str) {
		bool is_find = false;
		for (;;) {
			auto pos = sv.find("@P");
			if (pos != string_view::npos) {
				is_find = true;
				str.append(sv.substr(0, pos)).append("?");
				sv.remove_prefix(pos + 2);
				pos = sv.find_first_not_of("1234567890");
				if (pos != string_view::npos) {
					sv.remove_prefix(pos);
				}
				else {
					sv.remove_prefix(sv.size());
				}
			}
			else {
				break;
			}
		}

		if (!is_find) {
			for (;;) {
				auto pos = sv.find("@");
				if (pos != string_view::npos) {
					str.append(sv.substr(0, pos)).append("?");
					sv.remove_prefix(pos + 1);
					pos = sv.find_first_of(" =,)\r\n\t");
					if (pos != string_view::npos) {
						sv.remove_prefix(pos);
					}
					else {
						sv.remove_prefix(sv.size());
					}
				}
				else {
					break;
				}
			}
		}

		if (str.size()) {
			str.append(sv);
			return string_view(str);
		}
		else {
			return sv;
		}
	}

	string GetSHA256(const string& str, stringstream& ss) {
		string hash_string;
		EVP_MD_CTX* context = EVP_MD_CTX_new();
		if (context) {
			if (EVP_DigestInit_ex(context, EVP_sha256(), NULL)) {
				if (EVP_DigestUpdate(context, str.c_str(), str.length())) {
					unsigned char hash[EVP_MAX_MD_SIZE];
					unsigned int lengthOfHash = 0;
					if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
						for (unsigned int i = 0; i < lengthOfHash; ++i) {
							ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
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

	string GetSHA256(const string& str) {
		stringstream ss;
		return GetSHA256(str, ss);
	}
}
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "objects_event.h"

using namespace std;

namespace TechLogOneC {

	vector<sql_plan_token> PlanTxt::GetSqlPlanTokens() {
		vector<sql_plan_token> sql_plan_tokens(rows_all_.size());
		for (size_t index = 0; index < rows_all_.size(); ++index) {
			sql_plan_tokens[index] = {
				rows_all_[index].rows_,
				rows_all_[index].executes_,
				rows_all_[index].estimate_rows_,
				rows_all_[index].estimate_io_,
				rows_all_[index].estimate_cpu_,
				rows_all_[index].avg_row_size_,
				rows_all_[index].total_subtree_cost_,
				rows_all_[index].estimate_executions_,
				rows_all_[index].method_
			};
		}
		return sql_plan_tokens;
	}

	PlanTxt PlanTxt::Parse(std::string_view str) {
		PlanTxt plan_txt;
		auto pos = str.find('\n');
		while (pos != std::string_view::npos) {
			if (pos > 5) {
				PlanTxtRow plan_txt_row = PlanTxtRow::Parse(str.substr(0, pos));
				plan_txt.rows_ += plan_txt_row.rows_;
				plan_txt.executes_ += plan_txt_row.executes_;
				plan_txt.estimate_rows_ += plan_txt_row.estimate_rows_;
				plan_txt.estimate_io_ += plan_txt_row.estimate_io_;
				plan_txt.estimate_cpu_ += plan_txt_row.estimate_cpu_;
				plan_txt.estimate_executions_ += plan_txt_row.estimate_executions_;
				plan_txt.estimate_data_size_ += plan_txt_row.avg_row_size_ * plan_txt_row.estimate_rows_;
				plan_txt.data_size_ += plan_txt_row.avg_row_size_ * plan_txt_row.rows_;
				plan_txt.rows_all_.push_back(std::move(plan_txt_row));
			}
			str.remove_prefix(pos + 1);
			pos = str.find('\n');
		}
		return plan_txt;
	}

	PlanTxtRow PlanTxtRow::Parse(std::string_view str) {
		PlanTxtRow plan_txt_row;
		plan_txt_row.rows_ = GetInt64FromPlanSqlText(str);
		plan_txt_row.executes_ = GetInt64FromPlanSqlText(str);
		plan_txt_row.estimate_rows_ = GetDoubleFromPlanSqlText(str);
		plan_txt_row.estimate_io_ = GetDoubleFromPlanSqlText(str);
		plan_txt_row.estimate_cpu_ = GetDoubleFromPlanSqlText(str);
		plan_txt_row.avg_row_size_ = GetInt64FromPlanSqlText(str);
		plan_txt_row.total_subtree_cost_ = GetDoubleFromPlanSqlText(str);
		plan_txt_row.estimate_executions_ = GetDoubleFromPlanSqlText(str);
		plan_txt_row.stmt_text_ = GetStmtFromPlanSqlText(str);
		plan_txt_row.method_ = GetMethodFromPlanSqlText(str);
		return plan_txt_row;
	}

}
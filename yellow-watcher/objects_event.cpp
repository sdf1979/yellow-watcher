#include "objects_event.h"

using namespace std;

namespace TechLogOneC {

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
		plan_txt_row.stmt_text_ = str;
		return plan_txt_row;
	}

}
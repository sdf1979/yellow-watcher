#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "functions.h"

namespace TechLogOneC {

	struct PlanTxtRow {
		std::uint64_t rows_ = 0;
		std::uint64_t executes_ = 0;
		double estimate_rows_ = 0;
		double estimate_io_ = 0;
		double estimate_cpu_ = 0;
		std::uint64_t avg_row_size_ = 0;
		double total_subtree_cost_ = 0;
		double estimate_executions_ = 0;
		std::string stmt_text_;
		static PlanTxtRow Parse(std::string_view str);
	};

	struct PlanTxt {
		std::uint64_t rows_ = 0;
		std::uint64_t executes_ = 0;
		double estimate_rows_ = 0;
		double estimate_io_ = 0;
		double estimate_cpu_ = 0;
		double estimate_executions_ = 0;
		double estimate_data_size_ = 0;
		double data_size_ = 0;
		std::vector<PlanTxtRow> rows_all_;
		static PlanTxt Parse(std::string_view str);
	};

}

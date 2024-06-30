// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include<boost/program_options.hpp>
#include <string>
#include "encoding_string.h"

namespace opt = boost::program_options;

class ProgrammOptions {
	std::wstring mode;
	std::wstring path;
	std::wstring log_level;
	std::wstring help;
	std::wstring name_test;
	bool is_help;
	bool is_version;
public:
	ProgrammOptions(int argc, wchar_t* argv[]) {
		opt::options_description desc("All options");
		std::string mode_str("launch mode");
		mode_str
			.append(" (")
			.append("console - launch in the console")
			.append(", analysis - perform a complete analysis of all files")
			.append(", stress_test - loading stress testing data of all files")
			.append(", install - install the 'Yellow Watcher Service'")
			.append(", uninstall - remove the 'Yellow Watcher Service'")
			.append(")");

		desc.add_options()
			("mode,M", opt::wvalue<std::wstring>(&mode), mode_str.c_str())
			("name_test,N", opt::wvalue<std::wstring>(&name_test), "stress test description")
			("path,P", opt::wvalue<std::wstring>(&path), "directory with technological log files, takes precedence over the 'logs_path' parameter in the 'settings.json' file")
			("log,L", opt::wvalue<std::wstring>(&log_level)->default_value(L"error", "error"), "minimum level of logging (possible values ascending: trace, info, error)")
			("help,H", "produce help message")
			("version,V", "version");

		std::stringstream ss;
		ss << desc << '\n';
		help = Utf8ToWideChar(ss.str());

		opt::variables_map vm;
		try {
			opt::store(opt::wcommand_line_parser(argc, argv).options(desc).run(), vm);
			opt::notify(vm);
			is_help = vm.count("help");
			is_version = vm.count("version");
		}
		catch(...){
			is_help = true;
		}		
	}
	const std::wstring& Mode() { return mode; }
	const std::wstring& Path() { return path; }
	const std::wstring& LogLevel() { return log_level; }
	const std::wstring& Help() { return help; }
	const std::wstring& NameTest() { return name_test; }
	bool IsHelp() { return is_help; }
	bool IsVersion() { return is_version; }
};
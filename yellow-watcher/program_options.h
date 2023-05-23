﻿#pragma once

#include<boost/program_options.hpp>
#include <string>
#include "encoding_string.h"

namespace opt = boost::program_options;

class ProgrammOptions {
	std::wstring mode;
	std::wstring log_level;
	std::wstring help;
	bool is_help;
	bool is_version;
public:
	ProgrammOptions(int argc, wchar_t* argv[]) {
		opt::options_description desc("All options");
		desc.add_options()
			("mode,M", opt::wvalue<std::wstring>(&mode), "launch mode (console - launch in the console, install - install the 'Yellow Watcher Service', uninstall - remove the 'Yellow Watcher Service')")
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
	const std::wstring& LogLevel() { return log_level; }
	const std::wstring& Help() { return help; }
	bool IsHelp() { return is_help; }
	bool IsVersion() { return is_version; }
};
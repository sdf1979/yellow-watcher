#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <boost/json.hpp>

#include "encoding_string.h"


class Settings {
	std::string tech_logs_path;
	std::string service_path;
	std::string user;
	std::string password;
	std::string server;
	std::string port;
	std::string target;
	int log_storage_duration;
	void CreateSettings(const std::filesystem::path& file_path);
	void ParseServicePath();
public:
	bool Read(std::filesystem::path dir);
	std::string Server() { return server; }
	std::string Port() { return port; }
	std::string Target() { return target; }
	std::string User() { return user; }
	std::string Password() { return password; }
	std::string TechLogsPath() { return tech_logs_path; }
	int LogStorageDuration() { return log_storage_duration; }
};
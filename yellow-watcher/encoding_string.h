// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <windows.h>
#include <string>

std::wstring AnsiToWideChar(const std::string& str);
std::wstring Utf8ToWideChar(const std::string& str);
std::string WideCharToUtf8(const std::wstring& wstr);
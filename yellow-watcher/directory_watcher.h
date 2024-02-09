// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <memory>
#include <time.h>
#include <vector>

#include "reader.h"
#include "parser.h"
#include "logger.h"

using File = std::pair<std::unique_ptr<Reader>, std::unique_ptr<Parser>>;

class DirectoryWatcher {
    std::filesystem::path directory_;
    std::list<File> files_;
    std::unordered_map<std::wstring, std::list<File>::iterator> files_search_;
    time_t last_directory_read_;
    const time_t DIRECTORY_READ_PERIOD;
    std::vector<std::pair<std::tm, EventData>> events_;
    void AddFiles(const std::filesystem::path& path, bool check_file_time);
    void DeleteFiles(const std::vector<std::list<File>::iterator>& not_working_files);
    std::list<File>::iterator it_file_;
    std::vector<EventData>::iterator it_event_temp_;
    Reader* reader_;
    Parser* parser_;
    std::pair<const char*, int> buffer_;
    std::vector<EventData> events_temp_;
    std::int64_t read_bytes_;
public:
    DirectoryWatcher(std::wstring directory_name);
    void Init();
    void ExecuteStep(bool anyway);
    void ReadDirectory(bool anyway, bool check_file_time = true);
    void ReadFiles(bool check_file_time = true);
    bool OpenCursor();
    void CloseCursor();
    bool ReadNext(std::size_t count = 1000);
    const std::vector<std::pair<std::tm, EventData>>& GetEvents() const;
    std::vector<std::pair<std::tm, EventData>> MoveEvents();
    void ClearEvents();
    std::int64_t ReadBytes();
};

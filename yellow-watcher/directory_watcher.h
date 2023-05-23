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

    void AddFiles(std::filesystem::path path);
    void DeleteFiles(const std::vector<std::list<File>::iterator>& not_working_files);
    void ReadDirectory();
    void ReadFiles();
public:
    DirectoryWatcher(std::wstring directory_name);
    void Init();
    void ExecuteStep();
    const std::vector<std::pair<std::tm, EventData>>& GetEvents() const;
    void ClearEvents();
};

#include "directory_watcher.h"

static auto LOGGER = Logger::getInstance();

using namespace std;

DirectoryWatcher::DirectoryWatcher(wstring directory_name) :
    directory_(filesystem::path(directory_name)),
    last_directory_read_(0),
    DIRECTORY_READ_PERIOD(30) {}

void DirectoryWatcher::Init() {
    ReadDirectory();
    for (auto it = files_.begin(); it != files_.end(); ++it) {
        if (it->first->Open()) {
            it->first->FindStartPosition();
        }
    }
    ReadFiles();
    ClearEvents();
}

void DirectoryWatcher::ExecuteStep() {
    ReadDirectory();
    ReadFiles();
}

void DirectoryWatcher::AddFiles(filesystem::path path) {
    time_t cur_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
    wstring file_name = path.wstring();
    auto it = files_search_.find(file_name);
    if (it == files_search_.end()) {
        unique_ptr<Reader> new_file = make_unique<Reader>(file_name, 65535);
        if (new_file->IsWorkingFile(cur_time)) {
            files_.push_back({ move(new_file), make_unique<Parser>() });
            files_search_.insert({ file_name, --files_.end() });
            if (LOGGER->LogType() == Logger::Type::Trace) {
                string msg = "Added a new monitoring file '";
                msg.append(WideCharToUtf8(file_name)).append("\'");
                LOGGER->Print(msg, Logger::Type::Trace);
            }
        }
    };
}

void DirectoryWatcher::DeleteFiles(const vector<list<File>::iterator>& not_working_files) {
    for (auto& it : not_working_files) {
        wstring file_name(it->first->GetFileName());
        files_search_.erase(file_name);
        files_.erase(it);
        if (LOGGER->LogType() == Logger::Type::Trace) {
            string msg = "Removed file from monitoring '";
            msg.append(WideCharToUtf8(file_name)).append("\'");
            LOGGER->Print(msg, Logger::Type::Trace);
        }
    }
}

void DirectoryWatcher::ReadDirectory() {
    time_t cur_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
    if (cur_time - last_directory_read_ > DIRECTORY_READ_PERIOD) {
        try {
            for (const std::filesystem::directory_entry& dir : filesystem::recursive_directory_iterator(directory_)) {
                if (dir.is_regular_file() && dir.path().extension() == ".log") {
                    AddFiles(dir);
                }
            }
        }
        catch (std::system_error error) {
            string msg(directory_.string());
            msg.append(";").append(error.what());
            LOGGER->Print(AnsiToWideChar(msg), Logger::Type::Error);
        }
        last_directory_read_ = cur_time;
    }
}

void DirectoryWatcher::ReadFiles() {
    time_t cur_time = chrono::system_clock::to_time_t(chrono::system_clock::now());

    vector<list<File>::iterator> not_working_files;

    for (list<File>::iterator it = files_.begin(); it != files_.end(); ++it) {
        Reader* reader = it->first.get();
        Parser* parser = it->second.get();
        if (reader->Open()) {
            if (reader->Read()) {
                while (reader->Next()) {
                    auto buffer = reader->GetBuffer();
                    parser->Parse(buffer.first, buffer.second);
                    vector<EventData> events_temp = parser->MoveEvents();
                    for (auto it = events_temp.begin(); it != events_temp.end(); ++it) {
                        events_.push_back({ reader->FileTime(), move(*it) });
                    }
                    reader->ClearBuffer();
                }
            }
        }
        if (!it->first.get()->IsWorkingFile(cur_time)) {
            not_working_files.push_back(it);
        }
    }

    DeleteFiles(not_working_files);
}

const vector<pair<std::tm, EventData>>& DirectoryWatcher::GetEvents() const {
    return events_;
}

void DirectoryWatcher::ClearEvents() {
    events_.clear();
}
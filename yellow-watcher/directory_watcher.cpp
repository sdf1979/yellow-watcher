#include "directory_watcher.h"

static auto LOGGER = Logger::getInstance();

using namespace std;

DirectoryWatcher::DirectoryWatcher(wstring directory_name) :
    directory_(filesystem::path(directory_name)),
    last_directory_read_(0),
    DIRECTORY_READ_PERIOD(35),
    reader_(nullptr),
    parser_(nullptr),
    read_bytes_(0)
{}

void DirectoryWatcher::Init() {
    ReadDirectory(true);
    for (auto it = files_.begin(); it != files_.end(); ++it) {
        if (it->first->Open()) {
            it->first->FindStartPosition();
        }
    }
    ReadFiles();
    ClearEvents();
}

void DirectoryWatcher::ExecuteStep(bool anyway) {
    ReadDirectory(anyway);
    ReadFiles();
}

void DirectoryWatcher::AddFiles(filesystem::path path, bool check_file_time) {
    time_t cur_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
    wstring file_name = path.wstring();
    auto it = files_search_.find(file_name);
    if (it == files_search_.end()) {
        unique_ptr<Reader> new_file = make_unique<Reader>(file_name, 65535);
        if (new_file->IsWorkingFile(cur_time, check_file_time)) {
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

void DirectoryWatcher::ReadDirectory(bool anyway, bool check_file_time) {
    time_t cur_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
    if (anyway || cur_time - last_directory_read_ > DIRECTORY_READ_PERIOD) {
        try {
            for (const std::filesystem::directory_entry& dir : filesystem::recursive_directory_iterator(directory_)) {
                if (dir.is_regular_file() && dir.path().extension() == ".log") {
                    AddFiles(dir, check_file_time);
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

void DirectoryWatcher::ReadFiles(bool check_file_time) {
    time_t cur_time = chrono::system_clock::to_time_t(chrono::system_clock::now());

    read_bytes_ = 0;
    vector<list<File>::iterator> not_working_files;

    for (list<File>::iterator it = files_.begin(); it != files_.end(); ++it) {
        reader_ = it->first.get();
        parser_ = it->second.get();
        if (reader_->Open()) {
            if (reader_->Read()) {
                while (reader_->Next()) {
                    buffer_ = reader_->GetBuffer();
                    read_bytes_ += buffer_.second;
                    parser_->Parse(buffer_.first, buffer_.second);
                    events_temp_ = parser_->MoveEvents();
                    for (auto it = events_temp_.begin(); it != events_temp_.end(); ++it) {
                        events_.push_back({ reader_->FileTime(), move(*it) });
                    }
                    reader_->ClearBuffer();
                }
            }
        }
        if (!it->first.get()->IsWorkingFile(cur_time, check_file_time)) {
            not_working_files.push_back(it);
        }
        reader_ = nullptr;
        parser_ = nullptr;
    }

    DeleteFiles(not_working_files);
}

bool DirectoryWatcher::OpenCursor() {
    it_file_ = files_.begin();
    return true;
}

void DirectoryWatcher::CloseCursor() {
    reader_ = nullptr;
    parser_ = nullptr;
}

bool DirectoryWatcher::ReadNext(std::size_t count) {
    while (events_temp_.size() && it_event_temp_ != events_temp_.end()) {
        events_.push_back({ reader_->FileTime(), move(*it_event_temp_) });
        ++it_event_temp_;
        if (events_.size() >= count) return true;
    }
    while (it_file_ != files_.end()) {
        if (!reader_ && !parser_) {
            reader_ = it_file_->first.get();
            parser_ = it_file_->second.get();
            reader_->Open();
            reader_->Read();
        }
        while (reader_ && reader_->Next()) {
            buffer_ = reader_->GetBuffer();
            parser_->Parse(buffer_.first, buffer_.second);
            events_temp_ = parser_->MoveEvents();
            reader_->ClearBuffer();
            it_event_temp_ = events_temp_.begin();
            while (it_event_temp_ != events_temp_.end()) {
                events_.push_back({ reader_->FileTime(), move(*it_event_temp_) });
                ++it_event_temp_;
                if (events_.size() >= count) return true;
            }
        }
        reader_->Close();
        reader_ = nullptr;
        parser_ = nullptr;
        ++it_file_;
    }
    return events_.size();
}

const vector<pair<std::tm, EventData>>& DirectoryWatcher::GetEvents() const {
    return events_;
}

vector<pair<std::tm, EventData>> DirectoryWatcher::MoveEvents() {
    return move(events_);
}

void DirectoryWatcher::ClearEvents() {
    events_.clear();
}

std::int64_t DirectoryWatcher::ReadBytes() {
    return read_bytes_;
}
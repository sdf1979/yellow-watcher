#include "settings.h"

using namespace std;

namespace fs = std::filesystem;
namespace json = boost::json;

void Settings::CreateSettings(const fs::path& file_path) {
    if (!fs::exists(file_path)) {
        const std::string json = R"({ 
    "logs_path": "",
    "service_path": "http://server:port/base_name/hs/IntegrationService/LoadManagedLocks",
    "user": "",
    "password": "",
    "log_storage_duration_in_hours" : 24,
    "duration_long_request" : 3000000
})";
        ofstream out(file_path);
        out << json;
        out.close();
    }
}

void Settings::ParseServicePath() {
    string_view tmp(service_path);
    size_t pos = tmp.find("://");
    if (pos == string_view::npos) return;

    tmp.remove_prefix(pos + 3);
    pos = tmp.find(":");
    if (pos == string_view::npos) {
        pos = tmp.find("/");
        if (pos == string_view::npos) return;
        server = tmp.substr(0, pos);
        tmp.remove_prefix(pos);
        port = "80";
    }
    else {
        server = tmp.substr(0, pos);
        tmp.remove_prefix(pos + 1);
        pos = tmp.find("/");
        if (pos == string_view::npos) return;
        port = tmp.substr(0, pos);
        tmp.remove_prefix(pos);
    }
    target = tmp;
}

bool ReadValue(json::object* j_object, int& value, const char* key) {
    json::object::iterator it = j_object->find(key);
    if (it != j_object->cend()) {
        if (it->value().if_int64()) {
            value = static_cast<int>(it->value().as_int64());
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

bool ReadValue(json::object* j_object, uint64_t& value, const char* key) {
    json::object::iterator it = j_object->find(key);
    if (it != j_object->cend()) {
        if (it->value().if_int64()) {
            value = static_cast<uint64_t>(it->value().as_int64());
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

bool ReadValue(json::object* j_object, string& value, const char* key) {
    json::object::iterator it = j_object->find(key);
    if (it != j_object->cend()) {
        if (it->value().if_string()) {
            value = static_cast<string>(it->value().as_string());
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

bool ReadValue(json::object* j_object, vector<string>& value, const char* key) {
    json::object::iterator it = j_object->find(key);
    if (it != j_object->cend()) {
        if (it->value().if_array()) {
            json::array j_processes = it->value().as_array();
            for (auto it_process = j_processes.begin(); it_process < j_processes.end(); ++it_process) {
                if (it_process->if_string()) {
                    value.push_back(it_process->as_string().c_str());
                }
            }
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

bool Settings::Read(fs::path dir) {
    fs::path file_path = dir.append(L"settings.json");
    std::string path_settings = file_path.string();
    if (!fs::exists(file_path)) {
        CreateSettings(file_path);
    }

    wifstream in(file_path);
    in.imbue(std::locale("zh_CN.UTF-8"));
    if (in.is_open()) {
        wstringstream buffer;
        buffer.imbue(std::locale("zh_CN.UTF-8"));
        buffer << in.rdbuf();

        error_code ec;
        auto j_value = json::parse(WideCharToUtf8(buffer.str()), ec);
        if (ec) {
            //TODO LOGGER
            wcout << Utf8ToWideChar(ec.message());
            return false;
        }

        if (json::object* j_object = j_value.if_object()) {
            bool is_correct = true;
            is_correct = ReadValue(j_object, tech_logs_path, "logs_path") && is_correct;
            is_correct = ReadValue(j_object, service_path, "service_path") && is_correct;
            is_correct = ReadValue(j_object, user, "user") && is_correct;
            is_correct = ReadValue(j_object, password, "password") && is_correct;
            is_correct = ReadValue(j_object, log_storage_duration, "log_storage_duration_in_hours") && is_correct;
            is_correct = ReadValue(j_object, duration_long_request, "duration_long_request") && is_correct;
            if(is_correct) ParseServicePath();
            return is_correct;            
        }
        else {
            return false;
        }        
    }
    else {
        //TODO LOGGER
        return false;
    }
}
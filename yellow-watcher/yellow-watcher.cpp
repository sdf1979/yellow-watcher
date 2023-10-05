#include "sender.h"

#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <filesystem>
#include <thread>
#include "boost/json.hpp"

#include "program_options.h"
#include "settings.h"
#include "directory_watcher.h"
#include "reader.h"
#include "parser.h"
#include "timer_perf.h"
#include "encoding_string.h"
#include "logger.h"
#include "time.h"
#include "event_aggregator.h"
#include "event_accumulator.h"
#include "event_tech_log.h"
#include "objects_event.h"

static auto LOGGER = Logger::getInstance();
static std::filesystem::path PROGRAM_PATH;
static std::filesystem::path FILE_PATH;
static std::string HOST;

SERVICE_STATUS g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE g_ServiceThreadStop = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI WorkerThread(LPVOID lpParam);

wchar_t SERVICE_NAME[100] = L"Yellow Watcher Service";
const std::wstring* LOGS_PATH;
static const std::wstring VERSION = L"1.45";
static const std::string VERSION_STR = WideCharToUtf8(VERSION);

void GetPath();
void SetLoggerLevel(Logger* logger, const std::wstring& level);
void RunConsole();
void RunAnalysis();
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int InstallService(LPCWSTR serviceName, LPCWSTR servicePath);
int RemoveService(LPCWSTR serviceName);
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
DWORD WINAPI WorkerThread(LPVOID lpParam);

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    std::wstring host;
    host.resize(MAX_COMPUTERNAME_LENGTH + 1);
    DWORD host_size = static_cast<DWORD>(host.size());
    GetComputerNameW(host.data(), &host_size);
    HOST = WideCharToUtf8(host);

    ProgrammOptions program_options(argc, argv);
    if (program_options.IsHelp() || argc == 1) {
        std::wcout << program_options.Help() << '\n';
        return 0;
    }
    else if (program_options.IsVersion()) {
        std::wcout << L"Yellow Watcher v" << VERSION;
        return 0;
    }

    GetPath();
    LOGGER->Open(PROGRAM_PATH);
    SetLoggerLevel(LOGGER, program_options.LogLevel());

    LOGS_PATH = &program_options.Path();

    std::wstring mode = program_options.Mode();
    if (mode == L"console") {
        LOGGER->SetOutConsole(true);
        RunConsole();
        return 0;
    }
    else if (mode == L"analysis") {
        LOGGER->SetOutConsole(true);
        RunAnalysis();
        return 0;
    }
    else if (mode == L"install") {
        LOGGER->SetOutConsole(true);
        int result = InstallService(SERVICE_NAME, std::wstring(L"\"").append(FILE_PATH.wstring()).append(L"\" -M service").c_str());
        if (result == 0) {
            LOGGER->Print(std::wstring(L"Binary path: \"").append(FILE_PATH.wstring().append(L"\"")), true);
        }
        return result;
    }
    else if (mode == L"uninstall") {
        LOGGER->SetOutConsole(true);
        return RemoveService(SERVICE_NAME);
    }
    else if (mode == L"service") {
        std::wstring msg = L"Yellow Watcher v";
        msg.append(VERSION).append(L": run service mode");
        LOGGER->Print(msg, true);
    }
    else {
        return 0;
    }

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
        auto error = GetLastError();
        if (error == 1063) {
            LOGGER->SetOutConsole(true);
            LOGGER->Print("Set startup mode = console. For more details, see the help.", Logger::Type::Error);
            return 0;
        }
        else {
            LOGGER->Print(std::wstring(L"Yellow Watcher: Main: StartServiceCtrlDispatcher returned error ").append(std::to_wstring(error)), Logger::Type::Error);
        }
        return error;
    }

    std::wstring msg = L"Yellow Watcher v";
    msg.append(VERSION).append(L": stop service");
    LOGGER->Print(msg, true);

    return 0;
}

void RunConsole() {
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    std::wstring msg = L"Yellow Watcher: v";
    msg.append(VERSION).append(L": run console mode");
    LOGGER->Print(msg, true);
    
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent) {
        HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
        if (hThread) {
            std::wcout << L"Press Ctrl+C for exit\n";
            WaitForSingleObject(g_ServiceStopEvent, INFINITE);
            CloseHandle(g_ServiceStopEvent);
            CloseHandle(hThread);
        }
        g_ServiceStopEvent = INVALID_HANDLE_VALUE;
    }
    g_ServiceThreadStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceThreadStop) {
        WaitForSingleObject(g_ServiceThreadStop, 5000);
        CloseHandle(g_ServiceThreadStop);
    }
    msg = L"Yellow Watcher: v";
    msg.append(VERSION).append(L": stop console mode");
    LOGGER->Print(msg, true);
}

void RunAnalysis() {
    using namespace TechLogOneC;
    //ttimeout_aggregator  = count, time, database, last string context, context
    EventAggregator<std::uint64_t, std::uint64_t, std::string, std::string, std::string> ttimeout_aggregator;

    //tdeadlock_aggregator = count, time, database, last string context, context
    EventAggregator<std::uint64_t, std::uint64_t, std::string, std::string, std::string> tdeadlock_aggregator;

    //sql_aggregator = EventOptions, time, database, last string context, context, hash sql text
    EventAggregator<EventOptions, std::uint64_t, std::string, std::string, std::string, std::string> sql_aggregator;
    
    EventAccumulator<ManagedLockEvent> accumulator_managed_lock;

    DirectoryWatcher dw(*LOGS_PATH);
    const std::string* sql_text = nullptr;
    dw.ReadDirectory(true, false);
    if (dw.OpenCursor()) {
        while (dw.ReadNext()) {
            for (auto it = dw.GetEvents().begin(); it != dw.GetEvents().end(); ++it) {
                const tm& time = it->first;
                const EventData& event = it->second;
                if (event.Name() == "DBMSSQL") {
                    PlanTxt plan_txt = PlanTxt::Parse(*event.GetplanSQLText());
                    sql_text = event.GetSql();
                    if (sql_text->empty()) sql_text = event.GetFunc();
                    sql_aggregator.Add(
                        {
                            1,
                            event.Duration(),
                            RowsToUint64(event.GetRow()),
                            RowsToUint64(event.GetRowsAffected()),
                            plan_txt.rows_,
                            plan_txt.estimate_rows_,
                            plan_txt.data_size_,
                            plan_txt.estimate_data_size_
                        },
                        ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute()),
                        *event.GetDatabase(),
                        LastStringContext(event.GetContext()),
                        *event.GetContext(),
                        SqlHashDbMsSql(sql_text)
                    );
                }
                else if (event.Name() == "TTIMEOUT") {
                    ManagedLockEvent managed_lock_event(
                        ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute(), event.Second(), event.Msecond()),
                        event.Name(),
                        *event.GetDatabase(),
                        *event.GetComputer(),
                        *event.GetSession(),
                        *event.GetWaitConnections(),
                        *event.GetUsr(),
                        LastStringContext(event.GetContext()),
                        *event.GetContext()
                    );
                    ttimeout_aggregator.Add(1, RoundMinute(managed_lock_event.time_), managed_lock_event.data_base_, managed_lock_event.last_string_context_, managed_lock_event.context_);
                    accumulator_managed_lock.Add(std::move(managed_lock_event));
                }
            }
            dw.ClearEvents();
        }
        dw.CloseCursor();
    }
    ttimeout_aggregator.SetGenerationRule(
        GenerationRule{ KeyConversion::DateString, false },
        GenerationRule{ KeyConversion::NoConvertion, false },
        GenerationRule{ KeyConversion::NoConvertion, true },
        GenerationRule{ KeyConversion::NoConvertion , true });
    tdeadlock_aggregator.SetGenerationRule(
        GenerationRule{ KeyConversion::DateString, false },
        GenerationRule{ KeyConversion::NoConvertion, false },
        GenerationRule{ KeyConversion::NoConvertion, true },
        GenerationRule{ KeyConversion::NoConvertion , true });
    sql_aggregator.SetGenerationRule(
        GenerationRule{ KeyConversion::DateString, false },
        GenerationRule{ KeyConversion::NoConvertion, false },
        GenerationRule{ KeyConversion::NoConvertion, true },
        GenerationRule{ KeyConversion::NoConvertion , true },
        GenerationRule{ KeyConversion::NoConvertion , true });

    boost::json::object j_object;
    j_object.emplace("TTIMEOUT", ttimeout_aggregator.ToJsonObject());
    j_object.emplace("TDEADLOCK", tdeadlock_aggregator.ToJsonObject());
    j_object.emplace("DBMSSQL", sql_aggregator.ToJsonObject());
    j_object.emplace("MANAGED_LOCKS", accumulator_managed_lock.ToJsonObject());
    std::wcout << Utf8ToWideChar(boost::json::serialize(j_object)) << '\n';
}

void Send(Sender* sender, std::string target, std::string data) {
    std::chrono::time_point<std::chrono::high_resolution_clock> start_send = std::chrono::high_resolution_clock::now();
    sender->Send(target, data);

    if (LOGGER->IsTrace()) {
        auto duration_send = std::chrono::high_resolution_clock::now() - start_send;
        std::string msg = "Send: ";
        msg.append(std::to_string(duration_send.count())).append(" ns");
        LOGGER->Print(msg, Logger::Type::Trace);
    }
}

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    LOGGER->Print("Yellow Watcher: WorkerThread: Entry", Logger::Type::Trace);
    Settings settings;
    if (!settings.Read(PROGRAM_PATH)) {
        std::wstring msg = L"Can't read \'";
        msg.append(PROGRAM_PATH).append(L"\\settings.json file.");
        LOGGER->Print(msg, Logger::Type::Error);
        ExitProcess(1);
    }
    LOGGER->SetLogStorageDuration(settings.LogStorageDuration());
    if (!LOGS_PATH->empty()) settings.SetTechLogsPath(WideCharToUtf8(*LOGS_PATH));
    {
        time_point prevMinute = TechLogOneC::NowMinute();
        std::tm curTime = TechLogOneC::ToTm(prevMinute);
        
        DirectoryWatcher dw(Utf8ToWideChar(settings.TechLogsPath()));
        dw.Init();
        
        //ttimeout_aggregator  = count, time, database, computer, last string context, context
        TechLogOneC::EventAggregator<std::uint64_t, std::uint64_t, std::string, std::string, std::string, std::string> ttimeout_aggregator;

        //tdeadlock_aggregator = count, time, database, computer, last string context, context
        TechLogOneC::EventAggregator<std::uint64_t, std::uint64_t, std::string, std::string, std::string, std::string> tdeadlock_aggregator;

        //sql_aggregator       = EventOptions, time, database, computer, last string context, context, hash sql text
        TechLogOneC::EventAggregator<TechLogOneC::EventOptions, std::uint64_t, std::string, std::string, std::string, std::string, std::string> sql_aggregator;

        //tlock_aggregator     = Measurment, time, database, computer, regions
        TechLogOneC::EventAggregator<TechLogOneC::Measurment, std::uint64_t, std::string, std::string, std::string> tlock_aggregator;

        TechLogOneC::EventAccumulator<TechLogOneC::ManagedLockEvent> accumulator_managed_lock;
        Sender sender(settings.Server(), settings.Port(), settings.User(), settings.Password());

        std::stringstream ss;
        std::uint64_t max_event_time = 0;
        std::uint64_t cur_event_time;

        std::chrono::time_point<std::chrono::high_resolution_clock> start_read;
        DWORD res;
        const std::string* sql_text = nullptr;
        for (;;) {
            res = WaitForSingleObject(g_ServiceStopEvent, 0);
            if (g_ServiceStopEvent == INVALID_HANDLE_VALUE || res != WAIT_TIMEOUT) {
                break;
            }

            bool is_new_minute = prevMinute != TechLogOneC::RoundMinute(curTime);

            start_read = std::chrono::high_resolution_clock::now();
            dw.ExecuteStep(is_new_minute);
            for (auto it = dw.GetEvents().begin(); it != dw.GetEvents().end(); ++it) {
                const tm& time = it->first;
                const EventData& event = it->second;
                if (LOGGER->IsTrace()) {
                    cur_event_time = TechLogOneC::ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute(), event.Second(), event.Msecond());
                    if (cur_event_time > max_event_time) max_event_time = cur_event_time;
                }
                if (event.Name() == "DBMSSQL") {
                    TechLogOneC::PlanTxt plan_txt = TechLogOneC::PlanTxt::Parse(*event.GetplanSQLText());
                    sql_text = event.GetSql();
                    if (sql_text->empty()) sql_text = event.GetFunc();
                    sql_aggregator.Add(
                        {
                            1,
                            event.Duration(),
                            TechLogOneC::RowsToUint64(event.GetRow()),
                            TechLogOneC::RowsToUint64(event.GetRowsAffected()),
                            plan_txt.rows_,
                            plan_txt.estimate_rows_,
                            plan_txt.data_size_,
                            plan_txt.estimate_data_size_
                        },
                        TechLogOneC::ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute()),
                        *event.GetDatabase(),
                        *event.GetComputer(),
                        TechLogOneC::LastStringContext(event.GetContext()),
                        *event.GetContext(),
                        TechLogOneC::SqlHashDbMsSql(sql_text)
                    );
                }
                else if (event.Name() == "TTIMEOUT") {
                    TechLogOneC::ManagedLockEvent managed_lock_event(
                        TechLogOneC::ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute(), event.Second(), event.Msecond()),
                        event.Name(),
                        *event.GetDatabase(),
                        *event.GetComputer(),
                        *event.GetSession(),
                        *event.GetWaitConnections(),
                        *event.GetUsr(),
                        TechLogOneC::LastStringContext(event.GetContext()),
                        *event.GetContext());
                    ttimeout_aggregator.Add(
                        1,
                        TechLogOneC::RoundMinute(managed_lock_event.time_),
                        managed_lock_event.data_base_,
                        managed_lock_event.computer_,
                        managed_lock_event.last_string_context_,
                        managed_lock_event.context_);
                    accumulator_managed_lock.Add(std::move(managed_lock_event));
                }
                else if (event.Name() == "TDEADLOCK") {
                    TechLogOneC::ManagedLockEvent managed_lock_event(
                        TechLogOneC::ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute(), event.Second(), event.Msecond()),
                        event.Name(),
                        *event.GetDatabase(),
                        *event.GetComputer(),
                        *event.GetSession(),
                        TechLogOneC::WaitConnectionFromDeadlockConnectionIntersections(*event.GetDeadlockConnectionIntersections()),
                        *event.GetUsr(),
                        TechLogOneC::LastStringContext(event.GetContext()),
                        *event.GetContext()
                    );
                    tdeadlock_aggregator.Add(
                        1,
                        TechLogOneC::RoundMinute(managed_lock_event.time_),
                        managed_lock_event.data_base_,
                        managed_lock_event.computer_,
                        managed_lock_event.last_string_context_,
                        managed_lock_event.context_);
                    accumulator_managed_lock.Add(std::move(managed_lock_event));
                }
                else if (event.Name() == "TLOCK") {
                    tlock_aggregator.Add(
                        {
                            event.Duration(),
                            1
                        },
                        TechLogOneC::ToUint64(time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, event.Minute()),
                        *event.GetDatabase(),
                        HOST,
                        *event.GetRegions()
                    );
                }
            }
            dw.ClearEvents();

            if (LOGGER->IsTrace()) {
                auto duration_read = std::chrono::high_resolution_clock::now() - start_read;
                std::string msg = "Read maximum event time: ";
                msg.append(TechLogOneC::ToDateFormatString(max_event_time, ss))
                    .append(". Parse for: ").append(std::to_string(duration_read.count())).append(" ns");
                LOGGER->Print(msg, Logger::Type::Trace);
            }

            if (is_new_minute && curTime.tm_sec > 10) {
                std::uint64_t event_time = TechLogOneC::ToUint64(TechLogOneC::ToTm(prevMinute));
                boost::json::object j_object;
                j_object.emplace("version", VERSION_STR);
                j_object.emplace("date", TechLogOneC::ToDateFormatString(event_time, ss));
                
                auto ttimeout_aggregator_minute = ttimeout_aggregator.Find(event_time);
                if (ttimeout_aggregator_minute) {
                    ttimeout_aggregator_minute->SetGenerationRule(
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, false },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, false },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, true },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion , true });
                    j_object.emplace("ttimeout", ttimeout_aggregator_minute->ToJsonObject());
                    ttimeout_aggregator.Delete(event_time);
                }

                auto tdeadlock_aggregator_minute = tdeadlock_aggregator.Find(event_time);
                if (tdeadlock_aggregator_minute) {
                    tdeadlock_aggregator_minute->SetGenerationRule(
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, false },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, false },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, true },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion , true });
                    j_object.emplace("tdeadlock", tdeadlock_aggregator_minute->ToJsonObject());
                    tdeadlock_aggregator.Delete(event_time);
                }

                auto sql_aggregator_minute = sql_aggregator.Find(event_time);
                if (sql_aggregator_minute) {
                    sql_aggregator_minute->SetGenerationRule(
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, false },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, false },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion, true },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion , true },
                        TechLogOneC::GenerationRule{ TechLogOneC::KeyConversion::NoConvertion , true });
                    j_object.emplace("dbmssql", sql_aggregator_minute->ToJsonObject());
                    sql_aggregator.Delete(event_time);
                }

                auto tlock_aggregator_minute = tlock_aggregator.Find(event_time);
                if (tlock_aggregator_minute) {
                    j_object.emplace("tlock", tlock_aggregator_minute->ToJsonObject());
                    tlock_aggregator.Delete(event_time);
                }
                
                if (!accumulator_managed_lock.Empty()) {
                    j_object.emplace("managed_locks", accumulator_managed_lock.ToJsonObject());
                    accumulator_managed_lock.Clear();
                }

                std::thread thr_send(Send, &sender, std::move(settings.Target()), std::move(boost::json::serialize(j_object)));
                thr_send.detach();
                
                LOGGER->NewFileWithLock();
                prevMinute = TechLogOneC::RoundMinute(curTime);
            }

            if (dw.ReadBytes() == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (LOGGER->IsTrace()) LOGGER->Print("Sleep 500 ms", Logger::Type::Trace);
            }
            curTime = TechLogOneC::GetTime();
        }
    }

    if (LOGGER->IsTrace()) LOGGER->Print(L"Yellow Watcher: WorkerThread: Exit", Logger::Type::Trace);
    if (g_ServiceThreadStop != INVALID_HANDLE_VALUE) SetEvent(g_ServiceThreadStop);

    return ERROR_SUCCESS;
}

void GetPath() {
    WCHAR path[500];
    DWORD size = GetModuleFileNameW(NULL, path, 500);

    FILE_PATH = std::filesystem::path(path);
    PROGRAM_PATH = FILE_PATH.parent_path();
}

void SetLoggerLevel(Logger* logger, const std::wstring& level) {
    if (level == L"trace") logger->SetLogType(Logger::Type::Trace);
    else if (level == L"info") logger->SetLogType(Logger::Type::Info);
    else if (level == L"error") logger->SetLogType(Logger::Type::Error);
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
    if (!g_ServiceStopEvent) return false;
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
        LOGGER->Print(L"CTRL_C_EVENT", Logger::Type::Trace);
        SetEvent(g_ServiceStopEvent);
        break;
    case CTRL_BREAK_EVENT:
        LOGGER->Print(L"CTRL_BREAK_EVENT", Logger::Type::Trace);
        SetEvent(g_ServiceStopEvent);
        break;
    case CTRL_CLOSE_EVENT:
        LOGGER->Print(L"CTRL_CLOSE_EVENT", Logger::Type::Trace);
        SetEvent(g_ServiceStopEvent);
        break;
    case CTRL_LOGOFF_EVENT:
        LOGGER->Print(L"CTRL_LOGOFF_EVENT", Logger::Type::Trace);
        SetEvent(g_ServiceStopEvent);
        break;
    case CTRL_SHUTDOWN_EVENT:
        LOGGER->Print(L"CTRL_SHUTDOWN_EVENT", Logger::Type::Trace);
        SetEvent(g_ServiceStopEvent);
        break;
    }
    return true;
}

int InstallService(LPCWSTR serviceName, LPCWSTR servicePath) {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        std::wstring msg = L"Can't open Service Control Manager. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
        return -1;
    }

    SC_HANDLE hService = CreateService(
        hSCManager,
        serviceName,
        serviceName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        servicePath,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        std::wstring msg = L"Can't create service. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
        CloseServiceHandle(hSCManager);
        return -1;
    }

    SERVICE_DESCRIPTION info;
    info.lpDescription = LPWSTR(L"Yellow Watcher Service");
    ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &info);

    std::vector<SC_ACTION> types_actions = { {SC_ACTION_RESTART, 60000}, {SC_ACTION_RESTART, 120000 }, {SC_ACTION_RESTART, 60000} };
    SERVICE_FAILURE_ACTIONS sfa = { 300, NULL, NULL, static_cast<DWORD>(types_actions.size()), &types_actions[0] };
    ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    LOGGER->Print(L"Success install service!", true);

    return 0;
}

int RemoveService(LPCWSTR serviceName) {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        std::wstring msg = L"Can't open Service Control Manager. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
        return -1;
    }

    SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_STOP | DELETE);
    if (!hService) {
        std::wstring msg = L"Can't remove service. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
        CloseServiceHandle(hSCManager);
        return -1;
    }

    DeleteService(hService);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    LOGGER->Print(L"Success remove service!", true);

    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    DWORD Status = E_FAIL;

    LOGGER->Print("Yellow Watcher: ServiceMain: Entry", Logger::Type::Trace);

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (g_StatusHandle == NULL) {
        std::wstring msg = L"Yellow Watcher: ServiceMain: RegisterServiceCtrlHandler returned error. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
        return;
    }

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        std::wstring msg = L"Yellow Watcher: ServiceMain: SetServiceStatus returned error. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
    }

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        std::wstring msg = L"Yellow Watcher: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            std::wstring msg = L"Yellow Watcher: ServiceMain: SetServiceStatus returned error. ";
            msg.append(LOGGER->ErrorToString(GetLastError()));
            LOGGER->Print(msg, Logger::Type::Error);
        }
        LOGGER->Print(L"Yellow Watcher: ServiceMain: Exit", Logger::Type::Info, true);
        return;
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        std::wstring msg = L"Yellow Watcher: ServiceMain: SetServiceStatus returned error. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
    }

    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        std::wstring msg = L"Yellow Watcher: ServiceMain: SetServiceStatus returned error. ";
        msg.append(LOGGER->ErrorToString(GetLastError()));
        LOGGER->Print(msg, Logger::Type::Error);
    }

    LOGGER->Print(L"Yellow Watcher: ServiceMain: Exit", Logger::Type::Info, true);

    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
            break;
        }
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            LOGGER->Print(L"Yellow Watcher: ServiceCtrlHandler: SetServiceStatus returned error", Logger::Type::Error);
        }

        SetEvent(g_ServiceStopEvent);
        break;
    default:
        break;
    }
}


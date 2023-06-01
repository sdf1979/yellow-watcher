#include "sender.h"

#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <filesystem>

#include "program_options.h"
#include "settings.h"
#include "directory_watcher.h"
#include "reader.h"
#include "parser.h"
#include "ttimeout_aggregator.h"
#include "timer_perf.h"
#include "encoding_string.h"
#include "logger.h"
#include "time.h"

static auto LOGGER = Logger::getInstance();
static std::filesystem::path PROGRAM_PATH;
static std::filesystem::path FILE_PATH;

SERVICE_STATUS g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE g_ServiceThreadStop = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI WorkerThread(LPVOID lpParam);

wchar_t SERVICE_NAME[100] = L"Yellow Watcher Service";
static const std::wstring VERSION = L"1.3";

//TODO Delete
void RunConsole_old(Settings& settings);

void GetPath();
void SetLoggerLevel(Logger* logger, const std::wstring& level);
void RunConsole();
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int InstallService(LPCWSTR serviceName, LPCWSTR servicePath);
int RemoveService(LPCWSTR serviceName);
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
DWORD WINAPI WorkerThread(LPVOID lpParam);

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

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

    std::wstring mode = program_options.Mode();
    if (mode == L"console") {
        LOGGER->SetOutConsole(true);
        RunConsole();
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

    {
        time_point prevMinute = NowMinute();
        time_point nowMinute = prevMinute;
        
        DirectoryWatcher dw(Utf8ToWideChar(settings.TechLogsPath()));
        dw.Init();
        TtimeoutAggregator ttimeout_aggregator;
        TdeadlockAggregator tdeadlock_aggregator;
        Sender sender(settings.Server(), settings.Port(), settings.User(), settings.Password());

        DWORD res;
        for (;;) {
            res = WaitForSingleObject(g_ServiceStopEvent, 0);
            if (g_ServiceStopEvent == INVALID_HANDLE_VALUE || res != WAIT_TIMEOUT) {
                break;
            }

            if (prevMinute != nowMinute) {
                std::uint64_t event_time = ToUint64(ToTm(prevMinute));

                std::string tmp = ttimeout_aggregator.ToJson(event_time);
                sender.Send(settings.Target(), tmp);
                ttimeout_aggregator.Delete(event_time);
                ttimeout_aggregator.ClearEvents();

                tmp = tdeadlock_aggregator.ToJson(event_time);
                sender.Send(settings.Target(), tmp);
                tdeadlock_aggregator.Delete(event_time);
                tdeadlock_aggregator.ClearEvents();

                LOGGER->NewFileWithLock();

                prevMinute = nowMinute;
            }

            dw.ExecuteStep();
            for (auto it = dw.GetEvents().begin(); it != dw.GetEvents().end(); ++it) {
                if (it->second.Name() == "TTIMEOUT") {
                    ttimeout_aggregator.Add(it->first, it->second);
                }
                else if (it->second.Name() == "TDEADLOCK") {
                    tdeadlock_aggregator.Add(it->first, it->second);
                }
                if (LOGGER->LogType() == Logger::Type::Trace) {
                    std::string msg(it->second.Name());
                    msg.append(",SeesionId=")
                        .append(*it->second.GetSession());
                    LOGGER->Print(msg, Logger::Type::Trace);
                }
            }
            dw.ClearEvents();

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            nowMinute = NowMinute();
        }
    }

    LOGGER->Print(L"Yellow Watcher: WorkerThread: Exit", Logger::Type::Trace);
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

void RunConsole_old(Settings& settings) {

    //DirectoryWatcher dw(Utf8ToWideChar(SETTINGS.TechLogsPath()));
    //dw.Init();
    //dw.ExecuteStep();
    //int a = 1;

    //Reader reader(0, L"D:\\Сбермегамаркет РАБОТА\\LOG\\LOGS_RobotWork\\rphost_27060\\23050519.log", 1048576);
    //Reader reader(0, L"D:\\Сбермегамаркет РАБОТА\\LOG\\LOGS_RobotWork\\rphost_11240\\23050519.log", 1048576);
    Reader reader(L"D:\\Сбермегамаркет РАБОТА\\LOG\\TLOCKS_MONITORING\\rphost_11544\\23050520.log", 1048576);

    TtimeoutAggregator ttimeout_aggregator;

    Parser parser;
    if (reader.Open()) {
        MEASUREMENT;
        if (reader.Read()) {
            while (reader.Next()) {
                auto buffer = reader.GetBuffer();
                parser.Parse(buffer.first, buffer.second);
                std::vector<EventData> events_temp = parser.MoveEvents();
                for (auto it = events_temp.begin(); it < events_temp.end(); ++it) {
                    if (it->Name() == "TTIMEOUT") {
                        ttimeout_aggregator.Add(reader.FileTime(), *it);
                    }
                }
                reader.ClearBuffer();
            }
        }
    }
    std::string tmp = ttimeout_aggregator.ToJson();
    std::wstring w_tmp = Utf8ToWideChar(tmp);

    //std::string login = WideCharToUtf8(L"Администратор");

    //Sender sender(SETTINGS.Server(), SETTINGS.Port(), SETTINGS.User(), SETTINGS.Password());
    //sender.Send(SETTINGS.Target(), tmp);
}

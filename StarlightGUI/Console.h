#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <map>
#include <fstream>

#undef ERROR

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    OTHER
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::wstring message;
    std::wstring source;
};

class Console {
public:
    static Console& GetInstance();
    static void Destroy();

    bool Initialize();

    bool OpenConsole();
    bool CloseConsole();
    void ToggleConsole();

    // 彻底停止：线程退出、刷队列、关闭文件、释放控制台
    void Shutdown();

    void SetTitle(const std::wstring& title);
    void SetAppearanceByConfig();

    void SetShowTimestamp(bool show);
    void SetShowLogLevel(bool show);
    void SetShowSource(bool show);

    template<typename... Args>
    void Log(LogLevel level, const std::wstring& source, const std::wstring& message, Args&&... args) {
        wchar_t buffer[2048]{};
        int n = swprintf_s(buffer, _countof(buffer), message.c_str(), std::forward<Args>(args)...);
        std::wstring finalMsg = (n < 0) ? message : std::wstring(buffer);

        LogEntry entry;
        entry.timestamp = std::chrono::system_clock::now();
        entry.level = level;
        entry.message = std::move(finalMsg);
        entry.source = source;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_logQueue.push(entry);
        }
        m_queueCV.notify_one();

        {
            std::lock_guard<std::mutex> lock(m_fileQueueMutex);
            m_fileLogQueue.push(std::move(entry));
        }
        m_fileQueueCV.notify_one();
    }

    template<typename... Args>
    void Info(const std::wstring& source, const std::wstring& message, Args&&... args) {
        Log(LogLevel::INFO, source, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Warning(const std::wstring& source, const std::wstring& message, Args&&... args) {
        Log(LogLevel::WARNING, source, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Error(const std::wstring& source, const std::wstring& message, Args&&... args) {
        Log(LogLevel::ERROR, source, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Other(const std::wstring& source, const std::wstring& message, Args&&... args) {
        Log(LogLevel::Other, source, message, std::forward<Args>(args)...);
    }

    void SetMinLogLevel(LogLevel minLevel);

    void ClearConsole();

    bool SaveToFile(const std::wstring& filePath);

    void SetConsolePosition(int x, int y, int width, int height);

    std::vector<LogEntry> GetLogHistory();

    HWND GetConsoleHandle();

private:
    Console();
    ~Console();
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    void ConsoleThreadProc();
    void FileWriteThreadProc();

    void DrainConsoleQueue(std::queue<LogEntry>& local);
    void DrainFileQueue(std::queue<LogEntry>& local);

    void OutputToConsole(const LogEntry& entry);
    bool InitializeLogFile();

    WORD GetLevelColor(LogLevel level);
    void SetConsoleColor(WORD color);
    void ResetConsoleColor();

    std::wstring FormatTimestamp(const std::chrono::system_clock::time_point& time);
    std::wstring FormatLevel(LogLevel level);
    std::wstring FormatLogEntry(const LogEntry& entry);

private:
    std::atomic<bool> m_consoleOpen{ false };
    std::atomic<bool> m_initialized{ false };
    std::atomic<bool> m_shutdown{ false };
    std::atomic<LogLevel> m_minLogLevel{ LogLevel::INFO };

    HANDLE m_hConsoleOutput{ INVALID_HANDLE_VALUE };
    HANDLE m_hConsoleInput{ INVALID_HANDLE_VALUE };
    HWND m_hConsoleWnd{ nullptr };

    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::queue<LogEntry> m_logQueue;

    std::mutex m_fileQueueMutex;
    std::condition_variable m_fileQueueCV;
    std::queue<LogEntry> m_fileLogQueue;

    std::mutex m_historyMutex;
    std::deque<LogEntry> m_logHistory;
    size_t m_maxHistorySize{ 10000 };

    std::wofstream m_logFile;
    std::wstring m_logFilePath;
    std::mutex m_fileMutex;

    std::thread m_consoleThread;
    std::thread m_fileWriteThread;

    std::atomic<bool> m_showTimestamp{ true };
    std::atomic<bool> m_showLogLevel{ true };
    std::atomic<bool> m_showSource{ true };

    std::wstring m_consoleTitle{ L"Starlight GUI Logger" };

    std::map<LogLevel, WORD> m_colorMap{
        {LogLevel::INFO,     FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED},
        {LogLevel::WARNING,  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY},
        {LogLevel::ERROR,    FOREGROUND_RED | FOREGROUND_INTENSITY},
        {LogLevel::OTHER, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
    };
};

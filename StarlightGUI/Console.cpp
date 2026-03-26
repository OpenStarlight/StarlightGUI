#include "pch.h"
#include "Console.h"

#include <dwmapi.h>
#include <codecvt>
#include <locale>

using winrt::StarlightGUI::implementation::GetLocalizedString;

static Console* g_instance = nullptr;
static std::mutex g_instanceMutex;

static BOOL WINAPI HandleConsoleControl(DWORD type) {
    if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT || type == CTRL_C_EVENT) {
        // 这里只隐藏窗口，避免突然释放控制台资源导致奇怪行为
        Console::GetInstance().CloseConsole();
        return TRUE;
    }
    return FALSE;
}

Console& Console::GetInstance() {
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    if (!g_instance) {
        g_instance = new Console();
    }
    return *g_instance;
}

void Console::Destroy() {
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    if (g_instance) {
        delete g_instance;
        g_instance = nullptr;
    }
}

Console::Console() = default;

Console::~Console() {
    Shutdown();
}

bool Console::InitializeLogFile() {
    wchar_t tempPath[MAX_PATH]{};
    if (GetTempPathW(MAX_PATH, tempPath) == 0) {
        return false;
    }

    m_logFilePath = std::wstring(tempPath) + L"StarlightGUI.log";

    std::lock_guard<std::mutex> lock(m_fileMutex);
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
        return false;
    }

    try {
        m_logFile.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>()));
    }
    catch (...) {
    }

    m_logFile << L"========= Starlight GUI Log =========\n";
    m_logFile << t(L"Console_CreatedAt").c_str() << FormatTimestamp(std::chrono::system_clock::now()) << L"\n";
    m_logFile << t(L"Console_Version").c_str() << winrt::unbox_value<hstring>(Application::Current().Resources().TryLookup(box_value(L"Version"))) << L"\n";
    m_logFile << L"=====================================\n\n";
    m_logFile.flush();

    return true;
}

bool Console::Initialize() {
    if (m_initialized) return true;

    InitializeLogFile();

    BOOL result = AllocConsole();
    if (!result && GetLastError() != ERROR_ACCESS_DENIED) {
        return false;
    }

    // 重定向标准流
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    m_hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    m_hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    if (!m_hConsoleOutput || m_hConsoleOutput == INVALID_HANDLE_VALUE) {
        return false;
    }
    m_hConsoleWnd = GetConsoleWindow();

    DWORD dwMode = 0;
    if (GetConsoleMode(m_hConsoleOutput, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(m_hConsoleOutput, dwMode);
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SetConsoleCtrlHandler(HandleConsoleControl, TRUE);

    // 设置字体
    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    if (GetCurrentConsoleFontEx(m_hConsoleOutput, FALSE, &cfi)) {
        wcsncpy_s(cfi.FaceName, L"Consolas", _TRUNCATE);
        cfi.dwFontSize.Y = 16;
        cfi.FontWeight = FW_NORMAL;
        SetCurrentConsoleFontEx(m_hConsoleOutput, FALSE, &cfi);
    }

    if (m_hConsoleWnd) {
        // 隐藏
        ShowWindow(m_hConsoleWnd, SW_HIDE);

        // 禁用关闭按钮
        HMENU hMenu = GetSystemMenu(m_hConsoleWnd, FALSE);
        if (hMenu) EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED);

        // 设置标题
        SetConsoleTitleW(m_consoleTitle.c_str());
        
        // 设置背景
        SetBackdropByConfig();
    }

    m_consoleThread = std::thread(&Console::ConsoleThreadProc, this);
    m_fileWriteThread = std::thread(&Console::FileWriteThreadProc, this);

    Info(L"", L"Logger initialized. Closing console will terminate the application.");
    Info(L"", L"Log file: %s", m_logFilePath.c_str());

    m_shutdown = false;
    m_initialized = true;

    return true;
}

void Console::SetBackdropByConfig() {
    BOOL trueVal = TRUE;
    auto type = background_type == 0 ? DWMSBT_NONE : background_type == 1 ? (mica_type == 0 ? DWMSBT_MAINWINDOW : DWMSBT_TABBEDWINDOW) : DWMSBT_TRANSIENTWINDOW;
    MARGINS margins = { -1 };
    DwmSetWindowAttribute(m_hConsoleWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &trueVal, sizeof(trueVal));
    DwmSetWindowAttribute(m_hConsoleWnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
    DwmExtendFrameIntoClientArea(m_hConsoleWnd, &margins);
}

bool Console::OpenConsole() {
    if (!m_initialized) {
        if (!Initialize()) return false;
    }
    if (m_consoleOpen) return true;

    if (m_hConsoleWnd) {
        ShowWindow(m_hConsoleWnd, SW_SHOW);
        m_consoleOpen = true;
    }
    return m_consoleOpen;
}

bool Console::CloseConsole() {
    if (!m_consoleOpen) return true;

    if (m_hConsoleWnd) {
        ShowWindow(m_hConsoleWnd, SW_HIDE);
    }
    m_consoleOpen = false;
    return true;
}

void Console::ToggleConsole() {
    m_consoleOpen ? (void)CloseConsole() : (void)OpenConsole();
}

void Console::Shutdown() {
    bool expected = false;
    if (!m_shutdown.compare_exchange_strong(expected, true)) {
        return;
    }

    // 唤醒线程，让它们退出并刷完队列
    m_queueCV.notify_all();
    m_fileQueueCV.notify_all();

    if (m_consoleThread.joinable()) m_consoleThread.join();
    if (m_fileWriteThread.joinable()) m_fileWriteThread.join();

    // 关闭文件
    {
        std::lock_guard<std::mutex> lock(m_fileMutex);
        if (m_logFile.is_open()) {
            m_logFile.flush();
            m_logFile.close();
        }
    }

    // 释放控制台资源
    if (m_hConsoleWnd) {
        FreeConsole();
    }

    m_hConsoleOutput = INVALID_HANDLE_VALUE;
    m_hConsoleInput = INVALID_HANDLE_VALUE;
    m_hConsoleWnd = nullptr;

    m_consoleOpen = false;
    m_initialized = false;
}

void Console::SetTitle(const std::wstring& title) {
    m_consoleTitle = title;
    if (m_hConsoleWnd) {
        SetConsoleTitleW(title.c_str());
    }
}

void Console::ConsoleThreadProc() {
    std::queue<LogEntry> local;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this]() {
                return m_shutdown || !m_logQueue.empty();
                });

            if (m_shutdown && m_logQueue.empty()) break;

            std::swap(local, m_logQueue);
        }

        DrainConsoleQueue(local);
    }

    // 退出前再刷一次
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::swap(local, m_logQueue);
    }
    DrainConsoleQueue(local);
}

void Console::FileWriteThreadProc() {
    std::queue<LogEntry> local;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_fileQueueMutex);
            m_fileQueueCV.wait(lock, [this]() {
                return m_shutdown || !m_fileLogQueue.empty();
                });

            if (m_shutdown && m_fileLogQueue.empty()) break;

            std::swap(local, m_fileLogQueue);
        }

        DrainFileQueue(local);
    }

    // 退出前再刷一次
    {
        std::lock_guard<std::mutex> lock(m_fileQueueMutex);
        std::swap(local, m_fileLogQueue);
    }
    DrainFileQueue(local);
}

void Console::DrainConsoleQueue(std::queue<LogEntry>& local) {
    while (!local.empty()) {
        const LogEntry& entry = local.front();

        {
            std::lock_guard<std::mutex> lock(m_historyMutex);
            m_logHistory.push_back(entry);
            if (m_logHistory.size() > m_maxHistorySize) {
                m_logHistory.pop_front();
            }
        }

        if (m_initialized && entry.level >= m_minLogLevel) {
            OutputToConsole(entry);
        }

        local.pop();
    }
}

void Console::DrainFileQueue(std::queue<LogEntry>& local) {
    while (!local.empty()) {
        const LogEntry& entry = local.front();

        std::wstring formatted;
        try {
            formatted = FormatLogEntry(entry);
        }
        catch (...) {
            local.pop();
            continue;
        }

        std::lock_guard<std::mutex> lock(m_fileMutex);
        if (m_logFile.is_open()) {
            try {
                m_logFile << formatted << L"\n";
                m_logFile.flush();
            }
            catch (...) {
                // 写不进去就算啦
            }
        }

        local.pop();
    }
}

void Console::OutputToConsole(const LogEntry& entry) {
    if (!m_hConsoleOutput || m_hConsoleOutput == INVALID_HANDLE_VALUE) return;

    std::wstring output = FormatLogEntry(entry) + L"\n";

    SetConsoleColor(GetLevelColor(entry.level));

    DWORD charsWritten = 0;
    WriteConsoleW(m_hConsoleOutput, output.c_str(),
        static_cast<DWORD>(output.size()),
        &charsWritten, nullptr);

    ResetConsoleColor();
}

WORD Console::GetLevelColor(LogLevel level) {
    auto it = m_colorMap.find(level);
    if (it != m_colorMap.end()) return it->second;
    return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
}

void Console::SetConsoleColor(WORD color) {
    if (m_hConsoleOutput && m_hConsoleOutput != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(m_hConsoleOutput, color);
    }
}

void Console::ResetConsoleColor() {
    SetConsoleColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
}

std::wstring Console::FormatLogEntry(const LogEntry& entry) {
    std::wstringstream ss;

    if (m_showTimestamp) {
        ss << L"[" << FormatTimestamp(entry.timestamp) << L"] ";
    }
    if (m_showLogLevel) {
        ss << L"[" << FormatLevel(entry.level) << L"] ";
    }
    if (m_showSource && !entry.source.empty()) {
        ss << L"[" << entry.source << L"] ";
    }
    ss << entry.message;

    return ss.str();
}

std::wstring Console::FormatTimestamp(const std::chrono::system_clock::time_point& time) {
    auto t = std::chrono::system_clock::to_time_t(time);
    std::wstringstream ss;
    std::tm tm{};
    localtime_s(&tm, &t);
    ss << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::wstring Console::FormatLevel(LogLevel level) {
    switch (level) {
    case LogLevel::INFO:     return L"INFO";
    case LogLevel::WARNING:  return L"WARN";
    case LogLevel::ERROR:    return L"ERROR";
    case LogLevel::OTHER:    return L"OTHER";
    default:                 return L"UNKNOWN";
    }
}

void Console::SetMinLogLevel(LogLevel minLevel) {
    m_minLogLevel = minLevel;
}

void Console::ClearConsole() {
    if (!m_consoleOpen || !m_hConsoleOutput || m_hConsoleOutput == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    DWORD count = 0;
    DWORD cellCount = 0;
    COORD homeCoords{ 0, 0 };

    if (!GetConsoleScreenBufferInfo(m_hConsoleOutput, &csbi)) return;

    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    if (!FillConsoleOutputCharacterW(m_hConsoleOutput, L' ', cellCount, homeCoords, &count)) return;
    if (!FillConsoleOutputAttribute(m_hConsoleOutput, csbi.wAttributes, cellCount, homeCoords, &count)) return;

    SetConsoleCursorPosition(m_hConsoleOutput, homeCoords);
}

bool Console::SaveToFile(const std::wstring& filePath) {
    std::wofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;

    try {
        file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>()));
    }
    catch (...) {}

    std::lock_guard<std::mutex> lock(m_historyMutex);
    for (const auto& entry : m_logHistory) {
        file << FormatLogEntry(entry) << L"\n";
    }

    file.flush();
    file.close();
    return true;
}

void Console::SetConsolePosition(int x, int y, int width, int height) {
    if (!m_hConsoleWnd) return;
    MoveWindow(m_hConsoleWnd, x, y, width, height, TRUE);
}

std::vector<LogEntry> Console::GetLogHistory() {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    return std::vector<LogEntry>(m_logHistory.begin(), m_logHistory.end());
}

HWND Console::GetConsoleHandle() {
    return m_hConsoleWnd;
}

void Console::SetShowTimestamp(bool show) { m_showTimestamp = show; }
void Console::SetShowLogLevel(bool show) { m_showLogLevel = show; }
void Console::SetShowSource(bool show) { m_showSource = show; }


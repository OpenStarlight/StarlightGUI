#include "pch.h"
#include "CppUtils.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <random>
#include <limits>
#include <cwctype>
#include <fstream>

#undef max
#undef min

namespace winrt::StarlightGUI::implementation {
    const static uint64_t KB = 1024;
    const static uint64_t MB = KB * 1024;
    const static uint64_t GB = MB * 1024;

    std::wstring GenerateRandomString(size_t length) {
        const std::wstring charset =
            L"0123456789"
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            L"abcdefghijklmnopqrstuvwxyz";

        std::wstring result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            result += charset[GenerateRandomNumber(0, static_cast<int>(charset.size() - 1))];
        }
        return result;
    }

    int GenerateRandomNumber(size_t from, size_t to) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(from, to);

        return dist(gen);
    }

    int GetDateAsInt() {
        auto now = std::chrono::system_clock::now();

        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);

        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
            << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
            << std::setw(2) << std::setfill('0') << tm.tm_mday;

        return std::stoi(oss.str());
    }

    std::wstring FixBackSplash(const hstring& hstr) {
        std::wstring str = hstr.c_str();
        size_t pos = 0;
        while ((pos = str.find(L"\\\\")) != std::wstring::npos) {
            str.replace(pos, 2, L"\\");
        }
        return str;
    }

    std::wstring RemoveFromString(const hstring& hstr, const hstring& removeHstr) {
        std::wstring str = hstr.c_str();
        std::wstring removeStr = removeHstr.c_str();
        size_t pos = 0;
        while ((pos = str.find(removeHstr)) != std::wstring::npos) {
            str.replace(pos, removeHstr.size(), L"");
        }
        return str;
    }

    std::wstring GetParentDirectory(const hstring& path)
    {
        fs::path p(path.c_str());
        fs::path parent = p.parent_path();

        return parent.wstring();
    }

    std::string WideStringToString(const std::wstring& wstr)
    {
        const wchar_t* cstr = wstr.c_str();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, cstr, -1, nullptr, 0, nullptr, nullptr);

        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, cstr, -1, &str[0], size_needed, nullptr, nullptr);

        str.erase(std::find(str.begin(), str.end(), '\0'), str.end());

        return str;
    }

    std::wstring StringToWideString(const std::string& str)
    {
        const char* cstr = str.c_str();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, nullptr, 0);

        std::wstring widestr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, cstr, -1, &widestr[0], size_needed);

        widestr.erase(std::find(widestr.begin(), widestr.end(), L'\0'), widestr.end());

        return widestr;
	}

    std::wstring ULongToHexString(ULONG64 value)
    {
        return ULongToHexString(value, 16, true, true);
    }

    std::wstring ULongToHexString(ULONG64 value, int w, bool uppercase, bool prefix) {
        std::wstringstream ss;
        if (prefix) ss << L"0x";
        if (w > 0) ss << std::setw(w);
        ss << std::setfill(L'0') << std::hex;
        if (uppercase) ss << std::uppercase;
        ss << value;
        return ss.str();
    }

    bool HexStringToULong(const std::wstring& input, ULONG64& out)
    {
        out = 0;

        size_t begin = 0;
        while (begin < input.size() && iswspace(input[begin])) ++begin;

        size_t end = input.size();
        while (end > begin && iswspace(input[end - 1])) --end;

        if (begin >= end) return false;

        size_t i = begin;
        if ((end - i) >= 2 && input[i] == L'0' && (input[i + 1] == L'x' || input[i + 1] == L'X')) {
            i += 2;
            if (i >= end) return false;
        }

        ULONG64 value = 0;
        bool hasDigit = false;

        for (; i < end; ++i) {
            wchar_t c = input[i];
            if (!iswxdigit(c)) {
                return false;
            }

            hasDigit = true;

            int digit = 0;
            if (c >= L'0' && c <= L'9') digit = c - L'0';
            else {
                c = towupper(c);
                digit = 10 + (c - L'A'); // A-F
            }

            // 溢出检测
            if (value > (0xFFFFFFFFFFFFFFFFULL - (ULONG64)digit) / 16ULL) {
                return false;
            }

            value = value * 16ULL + (ULONG64)digit;
        }

        if (!hasDigit) return false;

        out = value;
        return true;
    }

    bool StringToNumber(const std::wstring& input, LONG64& out)
    {
        out = 0;

        size_t begin = 0;
        while (begin < input.size() && iswspace(input[begin])) ++begin;

        size_t end = input.size();
        while (end > begin && iswspace(input[end - 1])) --end;

        if (begin >= end) return false;

        bool negative = false;
        size_t i = begin;

        if (input[i] == L'+' || input[i] == L'-') {
            negative = (input[i] == L'-');
            ++i;
            if (i >= end) return false;
        }

        LONG64 value = 0;
        bool hasDigit = false;

        for (; i < end; ++i) {
            wchar_t c = input[i];
            if (!iswdigit(c)) {
                return false;
            }

            hasDigit = true;
            int digit = c - L'0';

            value = value * 10LL + digit;
        }

        if (!hasDigit) return false;

        out = negative ? -value : value;
        return true;
    }

    bool StringToNumber(const std::wstring& input, ULONG64& out)
    {
        out = 0;

        size_t begin = 0;
        while (begin < input.size() && iswspace(input[begin])) ++begin;

        size_t end = input.size();
        while (end > begin && iswspace(input[end - 1])) --end;

        if (begin >= end) return false;

        size_t i = begin;

        if (input[i] == L'-') {
            return false;
        }

        ULONG64 value = 0;
        bool hasDigit = false;

        for (; i < end; ++i) {
            wchar_t c = input[i];
            if (!iswdigit(c)) {
                return false;
            }

            hasDigit = true;
            int digit = c - L'0';

            value = value * 10ULL + digit;
        }

        if (!hasDigit) return false;

        out = value;
        return true;
    }

    std::wstring FormatMemorySize(double bytes)
    {
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1);

        if (bytes >= GB) {
            ss << bytes / GB << " GB";
        }
        else if (bytes >= MB) {
            ss << bytes/ MB << " MB";
        }
        else if (bytes >= KB) {
            ss << bytes / KB << " KB";
        }
        else {
            ss << bytes << " B";
        }

        return ss.str();
    }

    std::wstring ExtractFunctionName(const std::string& old) {
        std::wstring pretty(old.begin(), old.end());
        size_t firstNS = pretty.find(L"::");
        if (firstNS == std::string::npos) {
            return pretty;
        }

        size_t lastScope = pretty.rfind(L"::");
        if (lastScope == firstNS) {
            return pretty.substr(firstNS + 2);
        }

        size_t lastSecScope = pretty.rfind(L"::", lastScope - 2);
        if (lastSecScope != std::string::npos) {
            return pretty.substr(lastSecScope + 2);
        }

        return pretty;
    }

    std::wstring ExtractFileName(const std::wstring& path) {
        try {
            fs::path p(path);
            std::wstring filename = p.filename().wstring();
            return filename;
        }
        catch (const fs::filesystem_error& e) {
			return L"(δ֪)";
        }
    }

    std::wstring GetExecutablePath() {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        return exePath;
    }

    std::wstring GetInstalledLocationPath() {
        return fs::path(GetExecutablePath()).parent_path().wstring();
    }

    std::wstring GetSystemToolPath(const wchar_t* toolName)
    {
        wchar_t systemDir[MAX_PATH] = {};
        if (GetSystemDirectoryW(systemDir, MAX_PATH) > 0) {
            return std::wstring(systemDir) + L"\\" + toolName;
        }
        return std::wstring(L"C:\\Windows\\System32\\") + toolName;
    }

    int RunCommandHidden(std::wstring commandLine)
    {
        STARTUPINFOW si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(STARTUPINFOW);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            return -1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        return (int)exitCode;
    }

    bool RunSchtasks(std::wstring const& arguments)
    {
        std::wstring command = L"\"" + GetSystemToolPath(L"schtasks.exe") + L"\" " + arguments;
        return RunCommandHidden(command) == 0;
    }

    bool QueryTaskExists(std::wstring const& taskName)
    {
        std::wstring command = L"\"" + GetSystemToolPath(L"schtasks.exe") + L"\" /Query /TN \"" + taskName + L"\"";
        return RunCommandHidden(command) == 0;
    }

    bool WriteTextFile(std::wstring const& path, std::string const& content)
    {
        std::ofstream file(WideStringToString(path), std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(content.data(), (std::streamsize)content.size());
        file.close();
        return true;
    }

    std::wstring GetStacktrace(UINT length)
    {
        std::vector<PVOID> stack(length);
        WORD frames = RtlCaptureStackBackTrace(0, length, stack.data(), nullptr);
        std::wstringstream ss;
        for (WORD i = 0; i < frames; ++i) {
            ss << stack[i] << L" ";
        }
        return ss.str();
    }

    double GetValueFromCounter(PDH_HCOUNTER& counter) {
        PDH_FMT_COUNTERVALUE value;

        if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS) {
            return value.doubleValue;
        }

        return 0.0;
    }

    double GetValueFromCounterArray(PDH_HCOUNTER& counter) {
        double result = 0.0;
        DWORD bufferSize = 0;
        DWORD itemCount = 0;
        PDH_STATUS status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);

        std::vector<BYTE> buffer(bufferSize);
        PPDH_FMT_COUNTERVALUE_ITEM_W items =
            reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buffer.data());

        status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items);

        if (status != ERROR_SUCCESS) {
            return 0.0;
        }

        for (DWORD i = 0; i < itemCount; i++) {
            double value = items[i].FmtValue.doubleValue;
            result += value;
        }

        return result;
    }

    bool EnablePrivilege(LPCTSTR privilege) {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp{};

        if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            return false;
        }

        LookupPrivilegeValueW(NULL, privilege, &tkp.Privileges[0].Luid);

        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
        CloseHandle(hToken);

        return result != FALSE;
    }

    DWORD FindProcessId(const wchar_t* processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W pe = { sizeof(pe) };

        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName) == 0) {
                    CloseHandle(hSnapshot);
                    return pe.th32ProcessID;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }

        CloseHandle(hSnapshot);
        return 0;
    }

}

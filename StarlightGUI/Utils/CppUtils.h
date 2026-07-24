#pragma once

#include <Windows.h>
#include <pdh.h>
#include <string>
#include <string_view>
#include <winrt/Windows.Foundation.h>

namespace winrt::StarlightGUI::implementation {
    std::wstring GenerateRandomString(size_t length);

    int GenerateRandomNumber(size_t from, size_t to);

    int GetDateAsInt();

    std::wstring FixBackSplash(const hstring& hstr);

    std::wstring RemoveFromString(const hstring& hstr, const hstring& removeStr);

    std::wstring GetParentDirectory(const hstring& path);

    std::string WideStringToString(const std::wstring& str);

    std::wstring StringToWideString(const std::string& str);

    std::wstring ULongToHexString(ULONG64 value);

    std::wstring ULongToHexString(ULONG64 value, int w, bool uppercase, bool prefix);

    // The caller needs to distinguish invalid input from zero.
    bool HexStringToULong(const std::wstring& input, ULONG64& out);

    bool StringToNumber(const std::wstring& input, LONG64& out);

    bool StringToNumber(const std::wstring& input, ULONG64& out);

    std::wstring ToLowerCase(std::wstring_view input);

    int CompareIgnoreCase(std::wstring_view left, std::wstring_view right);

    bool LessIgnoreCase(std::wstring_view left, std::wstring_view right);

    bool ContainsIgnoreCaseLowerQuery(std::wstring_view text, std::wstring_view lowerQuery);

    bool ContainsIgnoreCase(std::wstring_view text, std::wstring_view query);

    std::wstring FormatMemorySize(double bytes);

    std::wstring ExtractFunctionName(const std::string& old);

    std::wstring ExtractFileName(const std::wstring& path);

    std::wstring GetExecutablePath();

    std::wstring GetInstalledLocationPath();

    std::wstring GetSystemToolPath(const wchar_t* toolName);

    int RunCommandHidden(std::wstring commandLine);

    bool RunSchtasks(std::wstring const& arguments);

    bool QueryTaskExists(std::wstring const& taskName);

    bool WriteTextFile(std::wstring const& path, std::string const& content);

    std::wstring GetStacktrace(UINT length);

    double GetValueFromCounter(PDH_HCOUNTER& counter);

    double GetValueFromCounterArray(PDH_HCOUNTER& counter);

    bool EnablePrivilege(LPCTSTR privilege);

    DWORD FindProcessId(const wchar_t* processName);
}

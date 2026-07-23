#pragma once

#include <Windows.h>
#include <cstddef>
#include <filesystem>
#include <string>

namespace slg
{
    std::filesystem::path GetExecutableDirectory();
    std::wstring GetDiagnosticTimestamp();
    void PruneDiagnosticFiles(
        std::filesystem::path const& directory,
        std::wstring const& extension,
        size_t maxFiles) noexcept;
    bool CreateCrashDump(EXCEPTION_POINTERS* exceptionPointers = nullptr) noexcept;
    void InitializeCrashHandler() noexcept;
}

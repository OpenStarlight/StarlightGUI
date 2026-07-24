#include "pch.h"
#include "Diagnostics.h"

#include <DbgHelp.h>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <vector>

#pragma comment(lib, "dbghelp.lib")

namespace slg
{
    namespace
    {
        constexpr size_t MAX_DUMP_FILES = 5;
        constexpr DWORD TERMINATE_EXCEPTION_CODE = 0xE0000001;
        std::atomic_flag dumpInProgress = ATOMIC_FLAG_INIT;

        struct DiagnosticFile
        {
            std::filesystem::path path;
            std::filesystem::file_time_type writeTime;
        };

        struct DumpContext
        {
            EXCEPTION_POINTERS* exceptionPointers;
            DWORD exceptionThreadId;
            bool success;
        };

        DWORD WINAPI DumpThreadProc(void* parameter) noexcept
        {
            auto context = (DumpContext*)parameter;
            if (!context) return ERROR_INVALID_PARAMETER;

            try {
                auto dumpDirectory = GetExecutableDirectory() / L"Dumps";
                std::error_code error;
                std::filesystem::create_directories(dumpDirectory, error);
                if (error) return error.value();

                auto dumpPath = dumpDirectory / (GetDiagnosticTimestamp() + L".dmp");
                HANDLE file = CreateFileW(
                    dumpPath.c_str(),
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    nullptr,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr);
                if (file == INVALID_HANDLE_VALUE) return GetLastError();

                MINIDUMP_EXCEPTION_INFORMATION exceptionInformation{};
                exceptionInformation.ThreadId = context->exceptionThreadId;
                exceptionInformation.ExceptionPointers = context->exceptionPointers;
                exceptionInformation.ClientPointers = FALSE;

                auto dumpType = (MINIDUMP_TYPE)(
                    MiniDumpWithDataSegs |
                    MiniDumpWithHandleData |
                    MiniDumpWithIndirectlyReferencedMemory |
                    MiniDumpWithThreadInfo |
                    MiniDumpWithUnloadedModules);

                context->success = MiniDumpWriteDump(
                    GetCurrentProcess(),
                    GetCurrentProcessId(),
                    file,
                    dumpType,
                    context->exceptionPointers ? &exceptionInformation : nullptr,
                    nullptr,
                    nullptr);
                DWORD dumpError = context->success ? ERROR_SUCCESS : GetLastError();

                FlushFileBuffers(file);
                CloseHandle(file);

                if (!context->success) {
                    std::filesystem::remove(dumpPath, error);
                    return dumpError;
                }

                PruneDiagnosticFiles(dumpDirectory, L".dmp", MAX_DUMP_FILES);
                return ERROR_SUCCESS;
            }
            catch (...) {
                return ERROR_UNHANDLED_EXCEPTION;
            }
        }

        LONG WINAPI CrashExceptionFilter(EXCEPTION_POINTERS* exceptionPointers) noexcept
        {
            CreateCrashDump(exceptionPointers);
            return EXCEPTION_EXECUTE_HANDLER;
        }

        [[noreturn]] void TerminateWithDump() noexcept
        {
            CreateCrashDump();
            TerminateProcess(GetCurrentProcess(), TERMINATE_EXCEPTION_CODE);
            std::abort();
        }
    }

    std::filesystem::path GetExecutableDirectory()
    {
        std::wstring path(32768, L'\0');
        DWORD length = GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size());
        if (length == 0 || length >= path.size()) return std::filesystem::current_path();

        path.resize(length);
        return std::filesystem::path(path).parent_path();
    }

    std::wstring GetDiagnosticTimestamp()
    {
        SYSTEMTIME time{};
        GetLocalTime(&time);

        wchar_t timestamp[32]{};
        swprintf_s(
            timestamp,
            L"%04u-%02u-%02u-%02u-%02u-%02u",
            time.wYear,
            time.wMonth,
            time.wDay,
            time.wHour,
            time.wMinute,
            time.wSecond);
        return timestamp;
    }

    void PruneDiagnosticFiles(
        std::filesystem::path const& directory,
        std::wstring const& extension,
        size_t maxFiles) noexcept
    {
        try {
            std::vector<DiagnosticFile> files;
            std::error_code error;

            if (!std::filesystem::exists(directory, error) || error) return;

            for (std::filesystem::directory_iterator iterator(directory, error), end;
                iterator != end && !error;
                iterator.increment(error)) {
                if (!iterator->is_regular_file(error) || error) {
                    error.clear();
                    continue;
                }
                if (_wcsicmp(iterator->path().extension().c_str(), extension.c_str()) != 0) continue;

                auto writeTime = iterator->last_write_time(error);
                if (error) {
                    error.clear();
                    writeTime = std::filesystem::file_time_type::min();
                }
                files.push_back({ iterator->path(), writeTime });
            }

            std::sort(files.begin(), files.end(), [](DiagnosticFile const& left, DiagnosticFile const& right) {
                if (left.writeTime != right.writeTime) return left.writeTime < right.writeTime;
                return left.path.filename().native() < right.path.filename().native();
                });

            size_t removeCount = files.size() - (std::min)(files.size(), maxFiles);
            for (size_t i = 0; i < removeCount; ++i) {
                std::filesystem::remove(files[i].path, error);
                error.clear();
            }
        }
        catch (...) {
        }
    }

    bool CreateCrashDump(EXCEPTION_POINTERS* exceptionPointers) noexcept
    {
        if (dumpInProgress.test_and_set()) return false;

        DumpContext context{
            exceptionPointers,
            GetCurrentThreadId(),
            false
        };

        HANDLE thread = CreateThread(nullptr, 0, DumpThreadProc, &context, 0, nullptr);
        if (thread) {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
        else {
            DumpThreadProc(&context);
        }

        dumpInProgress.clear();
        return context.success;
    }

    void InitializeCrashHandler() noexcept
    {
        try {
            auto dumpDirectory = GetExecutableDirectory() / L"Dumps";
            std::error_code error;
            std::filesystem::create_directories(dumpDirectory, error);
            PruneDiagnosticFiles(dumpDirectory, L".dmp", MAX_DUMP_FILES);
        }
        catch (...) {
        }

        SetUnhandledExceptionFilter(&CrashExceptionFilter);
        std::set_terminate(&TerminateWithDump);
        _set_purecall_handler(&TerminateWithDump);
        _set_invalid_parameter_handler([](
            wchar_t const*,
            wchar_t const*,
            wchar_t const*,
            unsigned int,
            uintptr_t) noexcept {
            TerminateWithDump();
            });
    }
}

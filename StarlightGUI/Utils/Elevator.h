#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "CppUtils.h"

using namespace winrt;
using namespace StarlightGUI::implementation;

inline bool EnableAllPrivileges(HANDLE tokenHandle) {
    DWORD bufferSize = 0;
    if (!GetTokenInformation(tokenHandle, TokenPrivileges, nullptr, 0, &bufferSize) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return false;

    std::vector<BYTE> buffer(bufferSize);
    PTOKEN_PRIVILEGES tokenPrivileges = (PTOKEN_PRIVILEGES)buffer.data();
    if (!GetTokenInformation(tokenHandle, TokenPrivileges, tokenPrivileges, bufferSize, &bufferSize))
        return false;

    for (DWORD i = 0; i < tokenPrivileges->PrivilegeCount; i++)
        tokenPrivileges->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(tokenHandle, FALSE, tokenPrivileges, bufferSize, nullptr, nullptr))
        return false;

    return GetLastError() == ERROR_SUCCESS;
}

inline bool CreateProcessElevated(std::wstring processName, bool fullPrivileges, std::wstring extraArgs = L"") {
    if (!EnablePrivilege(SE_DEBUG_NAME)) {
        LOG_ERROR(L"Elevator", L"Failed to obtain SE_DEBUG_PRIVILEGE.");
        return false;
    }

    if (!EnablePrivilege(SE_TCB_NAME)) {
        LOG_ERROR(L"Elevator", L"Failed to obtain SE_TCB_PRIVILEGE.");
        return false;
    }

    HANDLE systemToken = NULL;
    HANDLE impersonationToken = NULL;
    HANDLE trustedInstallerProcessToken = NULL;
    HANDLE trustedInstallerToken = NULL;
    HANDLE processHandle = NULL;
    HANDLE processToken = NULL;
    bool impersonating = false;

    auto cleanup = [&]() {
        if (trustedInstallerToken) CloseHandle(trustedInstallerToken);
        if (trustedInstallerProcessToken) CloseHandle(trustedInstallerProcessToken);
        if (processToken) CloseHandle(processToken);
        if (processHandle) CloseHandle(processHandle);
        if (systemToken) CloseHandle(systemToken);
        if (impersonationToken) CloseHandle(impersonationToken);
        if (impersonating) RevertToSelf();
    };

    DWORD winlogonPid = FindProcessId(L"winlogon.exe");
    if (!winlogonPid) {
        LOG_ERROR(L"Elevator", L"Failed to find winlogon.exe.");
        return false;
    }

    processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, winlogonPid);
    if (!processHandle) {
        LOG_ERROR(L"Elevator", L"Failed to open winlogon.exe.");
        return false;
    }

    if (!OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_QUERY, &processToken)) {
        LOG_ERROR(L"Elevator", L"Failed to get winlogon.exe token.");
        cleanup();
        return false;
    }

    CloseHandle(processHandle);
    processHandle = NULL;

    if (!DuplicateTokenEx(processToken, MAXIMUM_ALLOWED, nullptr,
        SecurityImpersonation, TokenPrimary, &systemToken)) {
        LOG_ERROR(L"Elevator", L"Failed to duplicate winlogon.exe primary token.");
        cleanup();
        return false;
    }

    if (!DuplicateTokenEx(processToken, MAXIMUM_ALLOWED, nullptr,
        SecurityImpersonation, TokenImpersonation, &impersonationToken)) {
        LOG_ERROR(L"Elevator", L"Failed to duplicate winlogon.exe impersonation token.");
        cleanup();
        return false;
    }

    CloseHandle(processToken);
    processToken = NULL;

    if (!ImpersonateLoggedOnUser(impersonationToken)) {
        LOG_ERROR(L"Elevator", L"Failed to impersonate SYSTEM.");
        cleanup();
        return false;
    }
    impersonating = true;

    SC_HANDLE serviceManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!serviceManager) {
        LOG_ERROR(L"Elevator", L"Failed to open service manager.");
        cleanup();
        return false;
    }

    SC_HANDLE service = OpenServiceW(serviceManager, L"TrustedInstaller", SERVICE_START | SERVICE_QUERY_STATUS);
    bool serviceStarted = service && (StartServiceW(service, 0, nullptr) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING);

    if (service) CloseServiceHandle(service);
    CloseServiceHandle(serviceManager);

    if (!serviceStarted) {
        std::wstring trustedInstallerPath = L"C:\\Windows\\servicing\\TrustedInstaller.exe";
        STARTUPINFOW startupInfo{ sizeof(startupInfo) };
        PROCESS_INFORMATION processInfo{};

        serviceStarted = CreateProcessAsUserW(systemToken, trustedInstallerPath.c_str(), nullptr,
            nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);

        if (processInfo.hThread) CloseHandle(processInfo.hThread);
        if (processInfo.hProcess) CloseHandle(processInfo.hProcess);
    }

    if (!serviceStarted) {
        LOG_ERROR(L"Elevator", L"Failed to start TrustedInstaller.");
        cleanup();
        return false;
    }

    DWORD trustedInstallerPid = 0;
    for (int i = 0; i < 10 && !trustedInstallerPid; i++) {
        trustedInstallerPid = FindProcessId(L"TrustedInstaller.exe");
        if (!trustedInstallerPid) Sleep(500);
    }

    if (!trustedInstallerPid) {
        LOG_ERROR(L"Elevator", L"Failed to find TrustedInstaller.exe.");
        cleanup();
        return false;
    }

    processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, trustedInstallerPid);
    if (!processHandle) {
        LOG_ERROR(L"Elevator", L"Failed to open TrustedInstaller.exe.");
        cleanup();
        return false;
    }

    if (!OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_QUERY, &trustedInstallerProcessToken)) {
        LOG_ERROR(L"Elevator", L"Failed to get TrustedInstaller.exe token.");
        cleanup();
        return false;
    }

    SECURITY_ATTRIBUTES securityAttributes{ sizeof(securityAttributes) };
    if (!DuplicateTokenEx(trustedInstallerProcessToken, TOKEN_ALL_ACCESS, &securityAttributes,
        SecurityImpersonation, TokenPrimary, &trustedInstallerToken)) {
        LOG_ERROR(L"Elevator", L"Failed to duplicate TrustedInstaller.exe token.");
        cleanup();
        return false;
    }

    CloseHandle(processHandle);
    processHandle = NULL;
    CloseHandle(trustedInstallerProcessToken);
    trustedInstallerProcessToken = NULL;

    if (fullPrivileges && !EnableAllPrivileges(trustedInstallerToken)) {
        LOG_ERROR(L"Elevator", L"Failed to enable all privileges.");
        cleanup();
        return false;
    }

    DWORD currentSessionId = WTSGetActiveConsoleSessionId();
    ProcessIdToSessionId(GetCurrentProcessId(), &currentSessionId);
    if (!SetTokenInformation(trustedInstallerToken, TokenSessionId, &currentSessionId, sizeof(currentSessionId))) {
        LOG_ERROR(L"Elevator", L"Failed to set token session.");
        cleanup();
        return false;
    }

    RevertToSelf();
    impersonating = false;

    STARTUPINFOW startupInfo{ sizeof(startupInfo) };
    PROCESS_INFORMATION processInfo{};
    std::wstring commandLine = L"\"" + processName + L"\"";
    if (!extraArgs.empty())
        commandLine += L" " + extraArgs;

    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_SHOW;

    bool created = CreateProcessWithTokenW(trustedInstallerToken, LOGON_WITH_PROFILE,
        processName.c_str(), commandLine.data(), 0, nullptr, nullptr, &startupInfo, &processInfo);

    if (!created) {
        LOG_WARNING(L"Elevator", L"CreateProcessWithTokenW() failed, trying CreateProcessAsUserW()...");
        created = CreateProcessAsUserW(trustedInstallerToken, processName.c_str(), commandLine.data(),
            nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);
    }

    if (processInfo.hThread) CloseHandle(processInfo.hThread);
    if (processInfo.hProcess) CloseHandle(processInfo.hProcess);
    cleanup();

    if (!created)
        LOG_ERROR(L"Elevator", L"CreateProcessAsUserW() failed.");

    return created;
}

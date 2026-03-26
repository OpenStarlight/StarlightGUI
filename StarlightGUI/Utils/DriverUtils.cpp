#include "pch.h"
#include "KernelBase.h"
#include <shellapi.h>

namespace winrt::StarlightGUI::implementation {
	bool DriverUtils::LoadKernelDriver(LPCWSTR kernelPath) noexcept {
		SC_HANDLE hSCM, hService;

		hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCM) {
			return false;
		}

		hService = OpenServiceW(hSCM, L"StarlightGUI Kernel Driver", SERVICE_ALL_ACCESS);
		if (hService) {
			// Start the service if it"s not running
			SERVICE_STATUS serviceStatus;
			if (!QueryServiceStatus(hService, &serviceStatus)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				return false;
			}

			if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
				LOG_INFO(L"Driver", L"Loading driver: %s", kernelPath);
				if (!StartServiceW(hService, 0, nullptr)) {
					CloseServiceHandle(hService);
					CloseServiceHandle(hSCM);
					return false;
				}
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		else {
			// Create the service
			hService = CreateServiceW(hSCM, L"StarlightGUI Kernel Driver", L"StarlightGUI Kernel Driver", SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
				SERVICE_ERROR_IGNORE, kernelPath, NULL, NULL, NULL,
				NULL, NULL);

			if (!hService) {
				CloseServiceHandle(hSCM);
				return false;
			}

			// Start the service
			LOG_INFO(L"Driver", L"Loading driver: %s", kernelPath);
			if (!StartServiceW(hService, 0, nullptr)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				return false;
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		return false;
	}

	bool DriverUtils::LoadDriver(LPCWSTR kernelPath, LPCWSTR fileName) noexcept {
		SC_HANDLE hSCM, hService;

		hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCM) {
			return false;
		}

		hService = OpenServiceW(hSCM, fileName, SERVICE_ALL_ACCESS);
		if (hService) {
			// Start the service if it"s not running
			SERVICE_STATUS serviceStatus;
			if (!QueryServiceStatus(hService, &serviceStatus)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				return false;
			}

			if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
				LOG_INFO(L"Driver", L"Loading driver: %s", kernelPath);
				if (!StartServiceW(hService, 0, nullptr)) {
					CloseServiceHandle(hService);
					CloseServiceHandle(hSCM);
					return false;
				}
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		else {
			// Create the service
			hService = CreateServiceW(hSCM, fileName, fileName, SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
				SERVICE_ERROR_IGNORE, kernelPath, NULL, NULL, NULL,
				NULL, NULL);

			if (!hService) {
				CloseServiceHandle(hSCM);
				return false;
			}

			// Start the service
			LOG_INFO(L"Driver", L"Loading driver: %s", kernelPath);
			if (!StartServiceW(hService, 0, nullptr)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				return false;
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		return false;
	}

	void DriverUtils::FixServices() noexcept {
		// 启动 fix_services.bat
		std::wstring path = GetInstalledLocationPath() + L"\\Assets\\fix_services.bat";
		SHELLEXECUTEINFOW sei = { sizeof(sei) };
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		sei.lpFile = path.c_str();
		sei.nShow = SW_SHOWNORMAL;
		sei.lpVerb = L"runas";

		BOOL stauts = ShellExecuteExW(&sei);

		if (stauts && sei.hProcess != NULL) {
			DWORD processId = GetProcessId(sei.hProcess);
			CloseHandle(sei.hProcess);
			CloseHandle(sei.hIcon);
			slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"),
				InfoBarSeverity::Success, g_mainWindowInstance);
		}
		else {
			slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()),
				InfoBarSeverity::Error, g_mainWindowInstance);
		}
	}
}

#include "pch.h"
#include "KernelBase.h"
#include "Utils.h"
#include <array>

namespace {
	bool LoadDriverService(LPCWSTR driverPath, LPCWSTR serviceName, LPCWSTR logSource, bool deleteOnFailure) noexcept {
		SC_HANDLE serviceManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
		if (!serviceManager) return false;

		DWORD serviceAccess = SERVICE_QUERY_STATUS | SERVICE_START;
		if (deleteOnFailure) serviceAccess |= DELETE;
		SC_HANDLE service = OpenServiceW(serviceManager, serviceName, serviceAccess);
		if (!service) {
			service = CreateServiceW(serviceManager, serviceName, serviceName, serviceAccess,
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
				driverPath, NULL, NULL, NULL, NULL, NULL);
		}

		if (!service) {
			CloseServiceHandle(serviceManager);
			return false;
		}

		SERVICE_STATUS serviceStatus{};
		bool result = QueryServiceStatus(service, &serviceStatus);
		if (result && serviceStatus.dwCurrentState == SERVICE_STOPPED) {
			LOG_INFO(logSource, L"Loading driver: %s", driverPath);
			result = StartServiceW(service, 0, nullptr) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;
		}

		if (!result && deleteOnFailure) DeleteService(service);

		CloseServiceHandle(service);
		CloseServiceHandle(serviceManager);
		return result;
	}
}

namespace winrt::StarlightGUI::implementation {
	bool DriverUtils::LoadKernelDriver(LPCWSTR kernelPath) noexcept {
		return LoadDriverService(kernelPath, L"Sirius for StarlightGUI", L"Sirius", true);
	}

	bool DriverUtils::LoadDriver(LPCWSTR kernelPath, LPCWSTR fileName) noexcept {
		return LoadDriverService(kernelPath, fileName, L"Driver", false);
	}

	bool DriverUtils::StopKernelDriver() noexcept {
		SC_HANDLE serviceManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
		if (!serviceManager) return false;

		SC_HANDLE service = OpenServiceW(serviceManager, L"Sirius for StarlightGUI", SERVICE_QUERY_STATUS | SERVICE_STOP);
		if (!service) {
			CloseServiceHandle(serviceManager);
			return false;
		}

		SERVICE_STATUS serviceStatus{};
		bool result = QueryServiceStatus(service, &serviceStatus);
		if (result && serviceStatus.dwCurrentState != SERVICE_STOPPED)
			result = ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus);

		CloseServiceHandle(service);
		CloseServiceHandle(serviceManager);
		return result;
	}

	void DriverUtils::FixServices() noexcept {
		SC_HANDLE serviceManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
		if (!serviceManager) return;

		constexpr std::array serviceNames{
			L"Sirius for StarlightGUI",
			L"StarlightGUI Kernel Driver",
			L"AstralX"
		};

		for (LPCWSTR serviceName : serviceNames) {
			SC_HANDLE service = OpenServiceW(serviceManager, serviceName, SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);
			if (!service) continue;

			SERVICE_STATUS serviceStatus{};
			if (QueryServiceStatus(service, &serviceStatus)) {
				if (serviceStatus.dwCurrentState != SERVICE_STOPPED)
					ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus);
				DeleteService(service);
			}
			CloseServiceHandle(service);
		}

		CloseServiceHandle(serviceManager);
		slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Settings.Msg.FixCompleted"), InfoBarSeverity::Success, g_mainWindowInstance);
	}
}

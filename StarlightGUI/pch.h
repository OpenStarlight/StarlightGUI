#pragma once

#include "BuildConfig.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "nvidia/nvml.lib")
#pragma comment(lib, "capstone/capstone.lib")

#undef GetCurrentTime
#undef min
#undef max
#undef CreateProcess
#undef LoadImage

#include <Console.h>
namespace winrt::StarlightGUI::implementation {
    std::wstring ExtractFunctionName(const std::string& old);
}

#define __WFUNCTION__ ExtractFunctionName(__FUNCTION__)
#define LOG_INFO(source, message, ...)     Console::GetInstance().Info(source, message, __VA_ARGS__)
#define LOG_WARNING(source, message, ...)  Console::GetInstance().Warning(source, message, __VA_ARGS__)
#define LOG_ERROR(source, message, ...)    Console::GetInstance().Error(source, message, __VA_ARGS__)
#define LOG_OTHER(source, message, ...)	   Console::GetInstance().Other(source, message, __VA_ARGS__)
#define LOGGER_INIT()			Console::GetInstance().Initialize()
#define LOGGER_TOGGLE()			Console::GetInstance().ToggleConsole()
#define LOGGER_OPEN()			Console::GetInstance().OpenConsole()
#define LOGGER_CLOSE()			Console::GetInstance().CloseConsole()
#define LOGGER_SHUTDOWN()		Console::GetInstance().Shutdown()

#include <Unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>

#include <Utils/I18n.h>
#include <Utils/DisplayValueConverter.h>

extern winrt::hstring siriusPath, wtmPath, iamKeyHackerPath;

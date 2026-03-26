#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "nvidia/nvml.lib")
#pragma comment(lib, "capstone/capstone.lib")

// 取消定义 GetCurrentTime 宏，避免与 Storyboard::GetCurrentTime 冲突
#undef GetCurrentTime
// 微软我超牛魔的
#undef min
#undef max

// 控制台相关
#include <Console.h>
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
#define LOGGER_CLEAR()			Console::GetInstance().ClearConsole()

#include <Unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Microsoft.Windows.Storage.Pickers.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Text.h>
#include <winrt/Microsoft.UI.h>
#include <wil/cppwinrt_helpers.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/WinUI3Package.h>

#include <Utils/ProcessInfo.h>
#include <Utils/ThreadInfo.h>
#include <Utils/HandleInfo.h>
#include <Utils/MokuaiInfo.h>
#include <Utils/KCTInfo.h>
#include <Utils/KernelModuleInfo.h>
#include <Utils/FileInfo.h>
#include <Utils/WindowInfo.h>

#include <Utils/ObjectEntry.h>
#include <Utils/GeneralEntry.h>

#include <Utils/TaskUtils.h>
#include <Utils/CppUtils.h>
#include <Utils/Utils.h>
#include <Utils/KernelBase.h>
#include <Utils/Elevator.h>
#include <Utils/Config.h>
#include <Utils/I18n.h>


extern winrt::hstring kernelPath, astralPath, wtmPath, iamKeyHackerPath;
extern std::wstring unused;
extern int enum_file_mode, background_type, mica_type, acrylic_type, navigation_style, image_stretch;
extern std::string background_image, language;
extern bool enum_strengthen, pdh_first, elevated_run, dangerous_confirm, check_update, task_auto_refresh, tray_background_run, auto_start, replace_taskmgr, navigate_task_request;
extern bool hypervisor_mode;
extern int image_opacity, disasm_count;

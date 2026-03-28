#include "pch.h"
#include "HomePage.xaml.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

#include <winrt/Windows.System.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <cwctype>
#include <iphlpapi.h>
#include <pdhmsg.h>
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Windows::Web::Http;
using namespace Windows::Data::Json;
using namespace Windows::System;
using namespace Windows::Storage;
using namespace Windows::Foundation;
using namespace Windows::Globalization;
using namespace Windows::ApplicationModel;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;

namespace winrt::StarlightGUI::implementation
{
    static double graphX = 1;

    HomePage::HomePage()
    {
        InitializeComponent();
        SetupLocalization();

        TotalLineGraph().AddSeries(L"CPU", Colors::LightSkyBlue());
        TotalLineGraph().AddSeries(t(L"Common.Memory"), Colors::DodgerBlue());
        TotalLineGraph().AddSeries(t(L"Common.Disk"), Colors::LimeGreen());
        TotalLineGraph().AddSeries(L"GPU", Colors::MediumPurple());

        this->Loaded([this](auto&&, auto&&) -> IAsyncAction {
            SetGreetingText();
            SetUserProfile();
            FetchHitokoto();
            SetupClock();
            infoInitialized = true;

            co_return;
            });

        this->Unloaded([this](auto&&, auto&&) {
            clockTimer.Stop();
            });

        LOG_INFO(L"HomePage", L"HomePage initialized.");
    }

    slg::coroutine HomePage::SetGreetingText()
    {
        if (greeting.empty()) {
            std::vector<hstring> greetings = {
                t(L"Home.WelcomeBack"),
                t(L"Home.Hello"),
                L"Hi",
                L"Ciallo～(∠・ω< )⌒★",
                L"TimeFormat",
            };
            greeting = greetings[GenerateRandomNumber(0, greetings.size() - 1)];

            auto now = std::chrono::system_clock::now();
            auto nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm localTime{};
            localtime_s(&localTime, &nowTime);
            int currentHour = localTime.tm_hour;

            if (greeting == L"TimeFormat") {
                if (currentHour >= 4 && currentHour < 12)
                {
                    greeting = t(L"Home.GoodMorning");
                }
                else if (currentHour < 18)
                {
                    greeting = t(L"Home.GoodAfternoon");
                }
                else if (currentHour < 4 || currentHour >= 18)
                {
                    greeting = t(L"Home.GoodEvening");
                }
            }
        }

        AppIntroduction().Text(t(L"Home.WelcomeMsg"));
        co_return;
    }

    slg::coroutine HomePage::SetUserProfile()
    {
        auto weak_this = get_weak();

        try {
            if (!infoInitialized) {
                co_await winrt::resume_background();

                LOG_INFO(__WFUNCTION__, L"Retrieving user profile...");

                auto users = co_await User::FindAllAsync(UserType::LocalUser, UserAuthenticationStatus::LocallyAuthenticated);

                if (users && users.Size() > 0)
                {
                    LOG_INFO(__WFUNCTION__, L"Retrieved user list.");
                    auto user = users.GetAt(0);

                    auto displayName = co_await user.GetPropertyAsync(KnownUserProperties::DisplayName());

                    if (displayName && !unbox_value<winrt::hstring>(displayName).empty())
                    {
                        username = unbox_value<winrt::hstring>(displayName);
                        LOG_INFO(__WFUNCTION__, L"Retrieved user account name successfully.");
                    }

                    auto picture = co_await user.GetPictureAsync(UserPictureSize::Size64x64);

                    if (picture)
                    {
                        LOG_INFO(__WFUNCTION__, L"Retrieved user picture.");
                        auto stream = co_await picture.OpenReadAsync();

                        if (stream)
                        {
                            co_await wil::resume_foreground(DispatcherQueue());

                            BitmapImage bitmapImage;
                            co_await bitmapImage.SetSourceAsync(stream);
                            avatar = bitmapImage;

                            LOG_INFO(__WFUNCTION__, L"Retrieved user account picture successfully.");
                        }
                    }
                }
            }

            if (auto strong_this = weak_this.get()) {
                co_await wil::resume_foreground(DispatcherQueue());
                UserAvatar().Name(username.cbegin());
                UserAvatar().ProfilePicture(avatar.as<winrt::Microsoft::UI::Xaml::Media::ImageSource>());
                g_mainWindowInstance->MainAvatar().Name(username.cbegin());
                g_mainWindowInstance->MainAvatar().ProfilePicture(avatar.as<winrt::Microsoft::UI::Xaml::Media::ImageSource>());
                WelcomeText().Text(greeting + L", " + username + L"!");
            }
        }
        catch (const hresult_error& e) {
            LOG_ERROR(__WFUNCTION__, L"Failed to retrieve user profile! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
            WelcomeText().Text(greeting + L"!");
        }
    }

    slg::coroutine HomePage::FetchHitokoto()
    {
        auto weak_this = get_weak();
        try {
            if (!infoInitialized) {
                co_await winrt::resume_background();

                LOG_INFO(__WFUNCTION__, L"Sending hitokoto request...");

                // 异步获取随机词条
                HttpClient client;
                /*
                * 移除：
                * a	动画
                * b	漫画
                * c	游戏
                * 因为太唐了受不了了 为什么说的话都那么逆天
                */
                Uri uri(L"https://v1.hitokoto.cn/?c=d&c=e&c=i&c=j&c=k");
                hstring result = co_await client.GetStringAsync(uri);

                // 读取 json 内容
                auto json = Windows::Data::Json::JsonObject::Parse(result);
                hitokoto = L"“" + json.GetNamedString(L"hitokoto") + L"”";

                LOG_INFO(__WFUNCTION__, L"Hitokoto json result: %s", result.c_str());
            }

            if (auto strong_this = weak_this.get()) {
                co_await wil::resume_foreground(DispatcherQueue());
                HitokotoText().Text(hitokoto);
            }
        }
        catch (const hresult_error& e) {
            LOG_ERROR(__WFUNCTION__, L"Failed to fetch hitokoto! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
            HitokotoText().Text(t(L"Msg.FetchFailed"));
        }
    }

    slg::coroutine HomePage::SetupClock()
    {
        UpdateClock();
        UpdateGauges();

        // 每秒更新一次
        clockTimer.Interval(std::chrono::seconds(1));
        clockTimer.Tick({ this, &HomePage::OnClockTick });
        clockTimer.Start();

        co_return;
    }

    slg::coroutine HomePage::OnClockTick(IInspectable const&, IInspectable const&)
    {
        if (!IsLoaded()) co_return;
        auto weak_this = get_weak();

        try {
            if (auto strong_this = weak_this.get()) {
                UpdateClock();
                UpdateGauges();
            }
            else {
                clockTimer.Stop();
            }
        }
        catch (const hresult_error& e) {
            LOG_ERROR(__WFUNCTION__, L"Error while clock ticking! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
        }
    }

    /*
    * ! 至尊答辩代码 !
    * @Author Stars
    */
    slg::coroutine HomePage::UpdateGauges() {
        if (!IsLoaded()) co_return;
        auto weak_this = get_weak();

        co_await winrt::resume_background();

        // 初始化性能计数器
        if (!initialized) {
            PdhOpenQueryW(NULL, 0, &query);
            PdhAddCounterW(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter_cpu_time);
            PdhAddCounterW(query, L"\\Processor Information(_Total)\\Actual Frequency", 0, &counter_cpu_freq);
            PdhAddCounterW(query, L"\\Processor Information(_Total)\\Actual Frequency", 0, &counter_cpu_freq);
            PdhAddCounterW(query, L"\\System\\Processes", 0, &counter_cpu_process);
            PdhAddCounterW(query, L"\\System\\Threads", 0, &counter_cpu_thread);
            PdhAddCounterW(query, L"\\System\\System Calls/sec", 0, &counter_cpu_syscall);
            PdhAddCounterW(query, L"\\Memory\\Cache Bytes", 0, &counter_mem_cached);
            PdhAddCounterW(query, L"\\Memory\\Committed Bytes", 0, &counter_mem_committed);
            PdhAddCounterW(query, L"\\Memory\\Page Reads/sec", 0, &counter_mem_read);
            PdhAddCounterW(query, L"\\Memory\\Page Writes/sec", 0, &counter_mem_write);
            PdhAddCounterW(query, L"\\Memory\\Pages Input/sec", 0, &counter_mem_input);
            PdhAddCounterW(query, L"\\Memory\\Pages Output/sec", 0, &counter_mem_output);
            PdhAddCounterW(query, L"\\PhysicalDisk(*)\\% Disk Time", 0, &counter_disk_time);
            PdhAddCounterW(query, L"\\PhysicalDisk(*)\\Disk Transfers/sec", 0, &counter_disk_trans);
            PdhAddCounterW(query, L"\\PhysicalDisk(*)\\Disk Read Bytes/sec", 0, &counter_disk_read);
            PdhAddCounterW(query, L"\\PhysicalDisk(*)\\Disk Write Bytes/sec", 0, &counter_disk_write);
            PdhAddCounterW(query, L"\\PhysicalDisk(*)\\Split IO/Sec", 0, &counter_disk_io);
            PdhAddCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter_gpu_time);

            LOG_INFO(L"MonitorInstance", L"Initialized PDH counters.");

            // 获取 CPU 型号
            int cpuInfo[4] = { 0 };
            char cpu_name[49] = { 0 };
            __cpuid(cpuInfo, 0x80000002);
            memcpy(cpu_name, cpuInfo, sizeof(cpuInfo));
            __cpuid(cpuInfo, 0x80000003);
            memcpy(cpu_name + 16, cpuInfo, sizeof(cpuInfo));
            __cpuid(cpuInfo, 0x80000004);
            memcpy(cpu_name + 32, cpuInfo, sizeof(cpuInfo));
            cpu_manufacture = to_hstring(cpu_name);

            // 获取 L1/L2/L3 缓存
            DWORD bufferSize = 0;
            GetLogicalProcessorInformation(NULL, &bufferSize);

            std::vector<BYTE> buffer(bufferSize);
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION slpi = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.data());
            GetLogicalProcessorInformation(slpi, &bufferSize);
            for (size_t i = 0; i < bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
                if (slpi[i].Relationship == RelationCache) {
                    if (slpi[i].Cache.Level == 1) cache_l1 += slpi[i].Cache.Size / 1024.0;
                    else if (slpi[i].Cache.Level == 2) cache_l2 += slpi[i].Cache.Size / (1024.0 * 1024.0);
                    else if (slpi[i].Cache.Level == 3) cache_l3 += slpi[i].Cache.Size / (1024.0 * 1024.0);
                }
            }

            LOG_INFO(L"MonitorInstance", L"Initialized CPU information.");

            // 获取 GPU 型号
            try {
                HMODULE hNvml = LoadLibraryW(L"nvml.dll");
                if (hNvml) {
                    FreeLibrary(hNvml);
                    if (nvmlInit_v2() == NVML_SUCCESS) {
                        UINT deviceCount = 0;
                        nvmlDeviceGetCount_v2(&deviceCount);
                        if (deviceCount > 0) {
                            isNvidia = true;
                            nvmlDeviceGetHandleByIndex_v2(0, &device);

                            char name[NVML_DEVICE_NAME_BUFFER_SIZE];
                            nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
                            gpu_manufacture = StringToWideString(name);

                            LOG_INFO(L"MonitorInstance", L"Initialized NVML.");
                        }
                        else {
                            LOG_ERROR(L"MonitorInstance", L"NVML return device count as 0!");
                            nvmlShutdown();
                        }
                    }
                }
            }
            catch (...) {
                isNvidia = false;
                LOG_ERROR(L"MonitorInstance", L"Failed to initialize NVML. Probably unsupported firmware. Try fallback solution.");
            }
            
            // 非 NVIDIA/不支持的设备，使用备用方案
            if (!isNvidia) {
                LOG_ERROR(L"MonitorInstance", L"Failed to initialize NVML. Probably unsupported firmware. Try fallback solution.");
                isNvidia = false;
                DISPLAY_DEVICEW dd{};
                dd.cb = sizeof(dd);
                for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); i++) {
                    if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE) {
                        gpu_manufacture = dd.DeviceString;
                        break;
                    }
                }
            }

            TrySelectActiveNetworkAdapter();

            initialized = true;
        }

        PdhCollectQueryData(query);

        // CPU
        ULONG64 seconds = GetTickCount64() / 1000;
        ULONG64 days = seconds / (24 * 3600);
        seconds %= (24 * 3600);
        ULONG64 hours = seconds / 3600;
        seconds %= 3600;
        ULONG64 minutes = seconds / 60;
        seconds %= 60;

        wchar_t timebuffer[256];
        swprintf_s(timebuffer, L"%llu:%02llu:%02llu:%02llu", days, hours, minutes, seconds);

        // 内存
        MEMORYSTATUSEX memInfo{};
        memInfo.dwLength = sizeof(memInfo);
        if (!GlobalMemoryStatusEx(&memInfo)) LOG_ERROR(L"MonitorInstance", L"Failed to get memory status.");

        // GPU
        nvmlUtilization_t gpu_utilization{};
        nvmlMemory_t gpu_memory{};
        UINT gpu_temp, gpu_clock_graphics, gpu_clock_mem;
        if (isNvidia) {
            nvmlDeviceGetUtilizationRates(device, &gpu_utilization);
            nvmlDeviceGetMemoryInfo(device, &gpu_memory);
            nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &gpu_temp);
            nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &gpu_clock_graphics);
            nvmlDeviceGetClockInfo(device, NVML_CLOCK_MEM, &gpu_clock_mem);
        }

        if (auto strong_this = weak_this.get()) {
            co_await wil::resume_foreground(DispatcherQueue());
            InitializeDiskCards();

            graphX += 1;

            std::wstringstream ss;
            CpuGauge().Value(GetValueFromCounter(counter_cpu_time));
            ss << std::fixed << std::setprecision(1) << GetValueFromCounter(counter_cpu_time) << "%";
            CpuPercent().Text(ss.str());
            CpuManufacture().Text(to_hstring(cpu_manufacture));
            ss = std::wstringstream{};
            ss << std::fixed << std::setprecision(2) << GetValueFromCounter(counter_cpu_freq) / 1024.0 << " GHz";
            CpuFrequency().Text(ss.str());
            CpuProcess().Text(to_hstring(GetValueFromCounter(counter_cpu_process)));
            CpuThread().Text(to_hstring(GetValueFromCounter(counter_cpu_thread)));
            ss = std::wstringstream{};
            ss << std::fixed << std::setprecision(1) << GetValueFromCounter(counter_cpu_syscall) << "/s";
            CpuSyscall().Text(ss.str());
            CpuRunTime().Text(timebuffer);
            CpuCore().Text(to_hstring(std::thread::hardware_concurrency()));
            CpuCacheL1().Text(to_hstring(cache_l1) + L" KB");
            CpuCacheL2().Text(to_hstring(cache_l2) + L" MB");
            CpuCacheL3().Text(to_hstring(cache_l3) + L" MB");
            TotalLineGraph().AddDataPoint(L"CPU", graphX, GetValueFromCounter(counter_cpu_time));

            MemGauge().Value(memInfo.dwMemoryLoad);
            MemPercent().Text(to_hstring((int)memInfo.dwMemoryLoad) + L"%");
            MemSize().Text(FormatMemorySize(memInfo.ullTotalPhys));
            MemUsing().Text(FormatMemorySize(memInfo.ullTotalPhys - memInfo.ullAvailPhys));
            MemUsable().Text(FormatMemorySize(memInfo.ullAvailPhys));
            MemCached().Text(FormatMemorySize(GetValueFromCounter(counter_mem_cached)));
            MemCommitted().Text(FormatMemorySize(GetValueFromCounter(counter_mem_committed)));
            MemPageRead().Text(FormatMemorySize(GetValueFromCounter(counter_mem_read)) + L"/s");
            MemPageWrite().Text(FormatMemorySize(GetValueFromCounter(counter_mem_write)) + L"/s");
            MemPageInput().Text(FormatMemorySize(GetValueFromCounter(counter_mem_input)) + L"/s");
            MemPageOutput().Text(FormatMemorySize(GetValueFromCounter(counter_mem_output)) + L"/s");
            TotalLineGraph().AddDataPoint(t(L"Common.Memory"), graphX, memInfo.dwMemoryLoad);

            auto diskTimeMap = GetDiskCounterMap(counter_disk_time);
            auto diskReadMap = GetDiskCounterMap(counter_disk_read);
            auto diskWriteMap = GetDiskCounterMap(counter_disk_write);
            auto diskTransMap = GetDiskCounterMap(counter_disk_trans);
            auto diskIoMap = GetDiskCounterMap(counter_disk_io);

            double totalDiskUsage = 0.0;
            size_t totalDiskCount = 0;

            for (auto& [index, card] : disk_card_map) {
                double timeValue = diskTimeMap[index];
                double readValue = diskReadMap[index];
                double writeValue = diskWriteMap[index];
                double transValue = diskTransMap[index];
                double ioValue = diskIoMap[index];

                card.gauge.Value(timeValue);

                ss = std::wstringstream{};
                ss << std::fixed << std::setprecision(1) << timeValue << "%";
                card.percent.Text(ss.str());
                ss = std::wstringstream{};
                ss << t(L"Home.Overview.ReadSpeed").c_str() << FormatMemorySize(readValue) << L"/s";
                card.read.Text(ss.str());
                ss = std::wstringstream{};
                ss << t(L"Home.Overview.WriteSpeed").c_str() << FormatMemorySize(writeValue) << L"/s";
                card.write.Text(ss.str());

                ss = std::wstringstream{};
                ss << t(L"Home.Overview.Transfer").c_str() << std::fixed << std::setprecision(1) << transValue << "/s";
                card.trans.Text(ss.str());

                ss = std::wstringstream{};
                ss << t(L"Home.Overview.IOCount").c_str() << std::fixed << std::setprecision(1) << ioValue << "/s";
                card.io.Text(ss.str());

                totalDiskUsage += timeValue;
                totalDiskCount++;
            }

            if (totalDiskCount > 0) totalDiskUsage /= static_cast<double>(totalDiskCount);
            TotalLineGraph().AddDataPoint(t(L"Common.Disk"), graphX, totalDiskUsage);

            double gpu_time = 0.0;
            if (isNvidia) {
                if (pdh_first) {
                    gpu_time = GetValueFromCounterArray(counter_gpu_time);
                }
                else {
                    gpu_time = gpu_utilization.gpu;
                }
                ss = std::wstringstream{};
                ss << std::fixed << std::setprecision(1) << FormatMemorySize(gpu_memory.used) << "/" << FormatMemorySize(gpu_memory.total);
                GpuMem().Text(ss.str());
                GpuTemp().Text(to_hstring(gpu_temp) + L" ℃");
                ss = std::wstringstream{};
                ss << std::fixed << std::setprecision(2) << gpu_clock_graphics / 1024.0 << " GHz";
                GpuClockGraphics().Text(ss.str());
                ss = std::wstringstream{};
                ss << std::fixed << std::setprecision(2) << gpu_clock_mem / 1024.0 << " GHz";
                GpuClockMem().Text(ss.str());
            }
            else {
                gpu_time = GetValueFromCounterArray(counter_gpu_time);
                GpuMem().Text(L"NaN");
                GpuTemp().Text(L"NaN");
                GpuClockGraphics().Text(L"NaN");
                GpuClockMem().Text(L"NaN");
            }
            GpuGauge().Value(gpu_time);
            ss = std::wstringstream{};
            ss << std::fixed << std::setprecision(1) << gpu_time << "%";
            GpuPercent().Text(ss.str());
            GpuManufacture().Text(gpu_manufacture);
            TotalLineGraph().AddDataPoint(L"GPU", graphX, gpu_time);

            double receiveBytesPerSec = 0.0, sendBytesPerSec = 0.0, receivePacketsPerSec = 0.0, sendPacketsPerSec = 0.0;
            if (!GetActiveNetworkSpeed(receiveBytesPerSec, sendBytesPerSec, receivePacketsPerSec, sendPacketsPerSec)) {
                TrySelectActiveNetworkAdapter();
            }

            if (isNetSend) {
                NetGauge().Value(sendBytesPerSec / (1024 * 1024));
                NetGauge().ValueStringFormat(L"↑ {0} MB/s");
            }
            else {
                NetGauge().Value(receiveBytesPerSec / (1024 * 1024));
                NetGauge().ValueStringFormat(L"↓ {0} MB/s");
            }
            NetManufacture().Text(netadpt_manufacture.empty() ? t(L"Home.Overview.NoActiveAdapter") : netadpt_manufacture);
            NetReceive().Text(FormatMemorySize(receiveBytesPerSec) + L"/s");
            NetSend().Text(FormatMemorySize(sendBytesPerSec) + L"/s");
            ss = std::wstringstream{};
            ss << std::fixed << std::setprecision(1) << receivePacketsPerSec << "/s";
            NetPacketReceive().Text(ss.str());
            ss = std::wstringstream{};
            ss << std::fixed << std::setprecision(1) << sendPacketsPerSec << "/s";
            NetPacketSend().Text(ss.str());
        }
    }

    slg::coroutine HomePage::UpdateClock()
    {
        if (!IsLoaded()) co_return;

        Calendar calendar;
        calendar.SetToNow();

        int hour = calendar.Hour();
        int minute = calendar.Minute();
        int second = calendar.Second();

        auto splitDigits = [](int value) -> std::pair<hstring, hstring> {
            int digit1 = value / 10;  // 十位
            int digit2 = value % 10;  // 个位
            return { to_hstring(digit1), to_hstring(digit2) };
            };

        auto hourDigits = splitDigits(hour);
        Hour1().Text(hourDigits.first);
        Hour2().Text(hourDigits.second);

        auto minuteDigits = splitDigits(minute);
        Minute1().Text(minuteDigits.first);
        Minute2().Text(minuteDigits.second);

        auto secondDigits = splitDigits(second);
        Second1().Text(secondDigits.first);
        Second2().Text(secondDigits.second);

        co_return;
    }

    void HomePage::ChangeMode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        isNetSend = !isNetSend;
        UpdateGauges();
    }

    bool HomePage::IsVirtualAdapterName(std::wstring const& name)
    {
        if (name.empty()) return false;
        static const std::vector<std::wstring> keys = {
            L"virtual", L"vmware", L"hyper-v", L"vethernet", L"tap", L"tun", L"vpn", L"loopback", L"pseudo", L"bridge", L"bluetooth"
        };

        for (auto const& key : keys) {
            if (ContainsIgnoreCaseLowerQuery(name, key)) return true;
        }
        return false;
    }

    bool HomePage::TrySelectActiveNetworkAdapter()
    {
        ULONG bestIfIndex = 0;
        GetBestInterface(0x08080808, &bestIfIndex);

        ULONG bufferSize = 0;
        if (GetAdaptersInfo(NULL, &bufferSize) != ERROR_BUFFER_OVERFLOW || bufferSize == 0) return false;

        std::vector<BYTE> buffer(bufferSize);
        auto adapters = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());
        if (GetAdaptersInfo(adapters, &bufferSize) != ERROR_SUCCESS) return false;

        PIP_ADAPTER_INFO selected = nullptr;
        PIP_ADAPTER_INFO fallback = nullptr;

        for (auto p = adapters; p; p = p->Next) {
            if (p->Type == MIB_IF_TYPE_LOOPBACK) continue;

            std::wstring desc = StringToWideString(p->Description ? p->Description : "");
            if (IsVirtualAdapterName(desc)) continue;

            MIB_IFROW row{};
            row.dwIndex = p->Index;
            if (GetIfEntry(&row) != NO_ERROR) continue;
            if (row.dwOperStatus != IF_OPER_STATUS_OPERATIONAL) continue;

            if (!fallback) fallback = p;
            if (p->Index == bestIfIndex) {
                selected = p;
                break;
            }
        }

        if (!selected) selected = fallback;
        if (!selected) {
            net_selected = false;
            netadpt_manufacture = L"";
            return false;
        }

        active_net_if_index = selected->Index;
        std::wstring adapterName = StringToWideString(selected->Description ? selected->Description : "");
        if (adapterName.empty()) adapterName = StringToWideString(selected->AdapterName ? selected->AdapterName : "");
        netadpt_manufacture = adapterName.empty() ? t(L"Home.Overview.UnknownAdapter") : hstring(adapterName);
        net_selected = true;

        MIB_IFROW row{};
        row.dwIndex = active_net_if_index;
        if (GetIfEntry(&row) == NO_ERROR) {
            last_in_octets = row.dwInOctets;
            last_out_octets = row.dwOutOctets;
            last_in_packets = row.dwInUcastPkts + row.dwInNUcastPkts;
            last_out_packets = row.dwOutUcastPkts + row.dwOutNUcastPkts;
            last_net_tick = GetTickCount64();
        }

        return true;
    }

    bool HomePage::GetActiveNetworkSpeed(double& receiveBytesPerSec, double& sendBytesPerSec, double& receivePacketsPerSec, double& sendPacketsPerSec)
    {
        receiveBytesPerSec = 0.0;
        sendBytesPerSec = 0.0;
        receivePacketsPerSec = 0.0;
        sendPacketsPerSec = 0.0;

        if (!net_selected && !TrySelectActiveNetworkAdapter()) return false;

        MIB_IFROW row{};
        row.dwIndex = active_net_if_index;
        if (GetIfEntry(&row) != NO_ERROR || row.dwOperStatus != IF_OPER_STATUS_OPERATIONAL) {
            net_selected = false;
            return false;
        }

        auto nowTick = GetTickCount64();
        if (last_net_tick == 0 || nowTick <= last_net_tick) {
            last_in_octets = row.dwInOctets;
            last_out_octets = row.dwOutOctets;
            last_in_packets = row.dwInUcastPkts + row.dwInNUcastPkts;
            last_out_packets = row.dwOutUcastPkts + row.dwOutNUcastPkts;
            last_net_tick = nowTick;
            return true;
        }

        double seconds = (nowTick - last_net_tick) / 1000.0;
        if (seconds <= 0.0) seconds = 1.0;

        UINT64 inOctets = row.dwInOctets;
        UINT64 outOctets = row.dwOutOctets;
        UINT64 inPackets = row.dwInUcastPkts + row.dwInNUcastPkts;
        UINT64 outPackets = row.dwOutUcastPkts + row.dwOutNUcastPkts;

        UINT64 inOctetsDelta = inOctets >= last_in_octets ? (inOctets - last_in_octets) : 0;
        UINT64 outOctetsDelta = outOctets >= last_out_octets ? (outOctets - last_out_octets) : 0;
        UINT64 inPacketsDelta = inPackets >= last_in_packets ? (inPackets - last_in_packets) : 0;
        UINT64 outPacketsDelta = outPackets >= last_out_packets ? (outPackets - last_out_packets) : 0;

        receiveBytesPerSec = static_cast<double>(inOctetsDelta) / seconds;
        sendBytesPerSec = static_cast<double>(outOctetsDelta) / seconds;
        receivePacketsPerSec = static_cast<double>(inPacketsDelta) / seconds;
        sendPacketsPerSec = static_cast<double>(outPacketsDelta) / seconds;

        last_in_octets = inOctets;
        last_out_octets = outOctets;
        last_in_packets = inPackets;
        last_out_packets = outPackets;
        last_net_tick = nowTick;
        return true;
    }

    int HomePage::ParseDiskIndexFromInstanceName(PCWSTR instanceName)
    {
        if (!instanceName || !instanceName[0]) return -1;
        if (wcscmp(instanceName, L"_Total") == 0) return -1;

        int index = 0;
        bool hasDigit = false;
        for (size_t i = 0; instanceName[i]; i++) {
            if (instanceName[i] >= L'0' && instanceName[i] <= L'9') {
                hasDigit = true;
                index = index * 10 + (instanceName[i] - L'0');
            }
            else {
                break;
            }
        }

        return hasDigit ? index : -1;
    }

    std::unordered_map<int, double> HomePage::GetDiskCounterMap(PDH_HCOUNTER& counter)
    {
        std::unordered_map<int, double> result;

        DWORD bufferSize = 0;
        DWORD itemCount = 0;
        PDH_STATUS status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);
        if (status != PDH_MORE_DATA || bufferSize == 0 || itemCount == 0) return result;

        std::vector<BYTE> buffer(bufferSize);
        auto items = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buffer.data());
        status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items);
        if (status != ERROR_SUCCESS) return result;

        for (DWORD i = 0; i < itemCount; i++) {
            int index = ParseDiskIndexFromInstanceName(items[i].szName);
            if (index < 0) continue;
            result[index] += items[i].FmtValue.doubleValue;
        }

        return result;
    }

    std::vector<int> HomePage::EnumerateDiskIndexes()
    {
        std::vector<int> indexes;

        DWORD counterListSize = 0;
        DWORD instanceListSize = 0;
        auto status = PdhEnumObjectItemsW(NULL, NULL, L"PhysicalDisk", NULL, &counterListSize, NULL, &instanceListSize, PERF_DETAIL_WIZARD, 0);
        if (status != PDH_MORE_DATA || instanceListSize == 0) return indexes;

        std::vector<wchar_t> counterList(counterListSize);
        std::vector<wchar_t> instanceList(instanceListSize);
        status = PdhEnumObjectItemsW(NULL, NULL, L"PhysicalDisk", counterList.data(), &counterListSize, instanceList.data(), &instanceListSize, PERF_DETAIL_WIZARD, 0);
        if (status != ERROR_SUCCESS) return indexes;

        for (const wchar_t* p = instanceList.data(); p && *p; p += wcslen(p) + 1) {
            int index = ParseDiskIndexFromInstanceName(p);
            if (index >= 0 && std::find(indexes.begin(), indexes.end(), index) == indexes.end()) {
                indexes.push_back(index);
            }
        }

        std::sort(indexes.begin(), indexes.end());
        return indexes;
    }

    hstring HomePage::QueryDiskManufacture(int diskIndex)
    {
        std::wstring path = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskIndex);
        HANDLE hDevice = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            LOG_ERROR(L"MonitorInstance", L"Failed to open %s.", path.c_str());
            return t(L"Home.Overview.UnknownModel");
        }

        STORAGE_PROPERTY_QUERY spq{};
        spq.PropertyId = StorageDeviceProperty;
        spq.QueryType = PropertyStandardQuery;

        std::vector<BYTE> buffer(1024);
        DWORD bytesReturned = 0;
        hstring manufacture = t(L"Home.Overview.UnknownModel");

        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesReturned, NULL))
        {
            auto sdd = reinterpret_cast<PSTORAGE_DEVICE_DESCRIPTOR>(buffer.data());
            if (sdd->ProductIdOffset != 0 && sdd->ProductIdOffset < buffer.size()) {
                LPCSTR productId = reinterpret_cast<LPCSTR>(buffer.data() + sdd->ProductIdOffset);
                manufacture = to_hstring(productId);
            }
        }
        else {
            LOG_ERROR(L"MonitorInstance", L"Failed to query disk info with %s.", path.c_str());
        }

        CloseHandle(hDevice);
        return manufacture;
    }

    void HomePage::BuildDiskCard(int diskIndex, hstring const& manufacture)
    {
        auto diskButton = Button();
        diskButton.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
        diskButton.HorizontalAlignment(HorizontalAlignment::Stretch);
        diskButton.HorizontalContentAlignment(HorizontalAlignment::Stretch);
        winrt::WinUI3Package::RevealFocusPanel::SetAttachToPanel(diskButton, RevealFocusPanel());

        auto grid = Grid();
        grid.Padding(ThicknessHelper::FromUniformLength(10));

        ColumnDefinition c1, c2, c3, c4;
        c1.Width(GridLengthHelper::FromPixels(150));
        c2.Width(GridLengthHelper::Auto());
        c3.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
        c4.Width(GridLengthHelper::Auto());
        grid.ColumnDefinitions().Append(c1);
        grid.ColumnDefinitions().Append(c2);
        grid.ColumnDefinitions().Append(c3);
        grid.ColumnDefinitions().Append(c4);

        auto gauge = winrt::XamlToolkit::WinUI::Controls::RadialGauge();
        gauge.Width(150);
        gauge.IsInteractive(false);
        gauge.Minimum(0);
        gauge.Maximum(100);
        gauge.StepSize(1);
        gauge.TickSpacing(10);
        gauge.NeedleBrush(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(Colors::LimeGreen()));
        gauge.TrailBrush(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(Colors::LimeGreen()));
        gauge.ValueStringFormat(L" {0}% ");
        Grid::SetColumn(gauge, 0);
        grid.Children().Append(gauge);

        auto infoPanel = StackPanel();
        infoPanel.Margin(ThicknessHelper::FromLengths(20, 0, 0, 0));
        Grid::SetColumn(infoPanel, 1);

        auto title = TextBlock();
        title.FontSize(20);
        title.FontWeight(winrt::Microsoft::UI::Text::FontWeights::Bold());
        std::wstringstream ss;
        ss << t(L"Common.DiskN", diskIndex).c_str();
        title.Text(ss.str());
        infoPanel.Children().Append(title);

        auto model = TextBlock();
        model.FontSize(16);
        model.FontWeight(winrt::Microsoft::UI::Text::FontWeights::SemiBold());
        model.Text(manufacture);
        infoPanel.Children().Append(model);

        auto readText = TextBlock();
        readText.Margin(ThicknessHelper::FromLengths(0, 10, 0, 0));
        readText.Text(t(L"Home.Overview.ReadSpeedZero"));
        readText.Foreground(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(Colors::LightGray()));
        infoPanel.Children().Append(readText);

        auto writeText = TextBlock();
        writeText.Text(t(L"Home.Overview.WriteSpeedZero"));
        writeText.Foreground(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(Colors::LightGray()));
        infoPanel.Children().Append(writeText);

        auto transText = TextBlock();
        transText.Text(t(L"Home.Overview.TransferZero"));
        transText.Foreground(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(Colors::LightGray()));
        infoPanel.Children().Append(transText);

        auto ioText = TextBlock();
        ioText.Text(t(L"Home.Overview.IOCountZero"));
        ioText.Foreground(winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(Colors::LightGray()));
        infoPanel.Children().Append(ioText);

        grid.Children().Append(infoPanel);

        auto percent = TextBlock();
        percent.FontSize(24);
        percent.FontWeight(winrt::Microsoft::UI::Text::FontWeights::Bold());
        percent.Text(L"0.0%");
        Grid::SetColumn(percent, 3);
        grid.Children().Append(percent);

        diskButton.Content(grid);
        DiskCardPanel().Children().Append(diskButton);

        DiskCardControl card{};
        card.index = diskIndex;
        card.manufacture = manufacture;
        card.gauge = gauge;
        card.title = title;
        card.read = readText;
        card.write = writeText;
        card.trans = transText;
        card.io = ioText;
        card.percent = percent;
        disk_card_map[diskIndex] = card;
    }

    void HomePage::InitializeDiskCards()
    {
        if (!disk_card_map.empty()) return;

        auto indexes = EnumerateDiskIndexes();
        if (indexes.empty()) {
            LOG_ERROR(L"MonitorInstance", L"No physical disk instance found.");
            return;
        }

        for (auto index : indexes) {
            hstring manufacture = QueryDiskManufacture(index);
            BuildDiskCard(index, manufacture);
        }
    }

    void HomePage::SetupLocalization() {
        HomeCurrentTimeUid().Text(t(L"Home.CurrentTime"));
        HomeAppIntroUid().Text(t(L"Home.Introduction"));
        HomeVersionLabelUid().Text(t(L"Home.VersionLabel"));
        HomeReleaseDateLabelUid().Text(t(L"Home.ReleaseDateLabel"));
        HomeDeveloperLabelUid().Text(t(L"Home.DeveloperLabel"));
        HomeOpenSourceUid().Text(t(L"Home.OpenSource"));
        HomeOverviewUid().Text(t(L"Home.Overview"));
        HomeChartUid().Header(tbox(L"Home.Overview.Chart"));
        HomeCpuSpeedUid().Text(t(L"Home.Overview.CpuSpeed"));
        HomeCpuProcessUid().Text(t(L"Home.Overview.CpuProcess"));
        HomeCpuThreadUid().Text(t(L"Home.Overview.CpuThread"));
        HomeCpuSyscallUid().Text(t(L"Home.Overview.CpuSyscall"));
        HomeCpuUptimeUid().Text(t(L"Home.Overview.CpuUptime"));
        HomeCpuCoresUid().Text(t(L"Home.Overview.CpuCores"));
        HomeCpuCacheL1Uid().Text(t(L"Home.Overview.CpuCacheL1"));
        HomeCpuCacheL2Uid().Text(t(L"Home.Overview.CpuCacheL2"));
        HomeCpuCacheL3Uid().Text(t(L"Home.Overview.CpuCacheL3"));
        HomeMemTitleUid().Text(t(L"Home.Overview.MemTitle"));
        HomeMemInUseUid().Text(t(L"Home.Overview.MemInUse"));
        HomeMemAvailableUid().Text(t(L"Home.Overview.MemAvailable"));
        HomeMemCommittedUid().Text(t(L"Home.Overview.MemCommitted"));
        HomeMemCachedUid().Text(t(L"Home.Overview.MemCached"));
        HomeMemPageReadUid().Text(t(L"Home.Overview.MemPageRead"));
        HomeMemPageWriteUid().Text(t(L"Home.Overview.MemPageWrite"));
        HomeMemPageOutUid().Text(t(L"Home.Overview.MemPageOut"));
        HomeMemPageInUid().Text(t(L"Home.Overview.MemPageIn"));
        HomeGpuMemUid().Text(t(L"Home.Overview.GpuMem"));
        HomeGpuTempUid().Text(t(L"Home.Overview.GpuTemp"));
        HomeGpuClockGraphicsUid().Text(t(L"Home.Overview.GpuClockGraphics"));
        HomeGpuClockMemUid().Text(t(L"Home.Overview.GpuClockMem"));
        HomeNetTitleUid().Text(t(L"Home.Overview.NetTitle"));
        HomeNetDownloadUid().Text(t(L"Home.Overview.NetDownload"));
        HomeNetUploadUid().Text(t(L"Home.Overview.NetUpload"));
        HomeNetPacketSendUid().Text(t(L"Home.Overview.NetPacketSend"));
        HomeNetPacketRecvUid().Text(t(L"Home.Overview.NetPacketRecv"));
        HomeChangeModeUid().Content(tbox(L"Home.Overview.ChangeMode"));
    }
}




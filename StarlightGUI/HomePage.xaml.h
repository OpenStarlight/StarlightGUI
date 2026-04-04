#pragma once

#include "HomePage.g.h"
#include <pdh.h>
#include <nvidia/nvml.h>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>
#include <winrt/XamlToolkit.WinUI.Controls.h>

namespace winrt::StarlightGUI::implementation
{
    struct DiskCardControl
    {
        int index = 0;
        hstring manufacture = L"";
        winrt::XamlToolkit::WinUI::Controls::RadialGauge gauge{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock title{ nullptr };
        winrt::Microsoft::UI::Xaml::Documents::Run read{ nullptr };
        winrt::Microsoft::UI::Xaml::Documents::Run write{ nullptr };
        winrt::Microsoft::UI::Xaml::Documents::Run trans{ nullptr };
        winrt::Microsoft::UI::Xaml::Documents::Run io{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock percent{ nullptr };
    };

    struct HomePage : public HomePageT<HomePage>
    {
        HomePage();
		void SetupLocalization();

        void SetGreetingText();
        slg::coroutine SetUserProfile();
        slg::coroutine FetchHitokoto();
        void SetupClock();
        slg::coroutine OnClockTick(IInspectable const&, IInspectable const&);
        slg::coroutine UpdateClock();

        slg::coroutine UpdateGauges();
        void InitializeDiskCards();
        void BuildDiskCard(int diskIndex, hstring const& manufacture);
        std::unordered_map<int, double> GetDiskCounterMap(PDH_HCOUNTER& counter);
        std::vector<int> EnumerateDiskIndexes();
        hstring QueryDiskManufacture(int diskIndex);
        int ParseDiskIndexFromInstanceName(PCWSTR instanceName);
        bool TrySelectActiveNetworkAdapter();
        bool IsVirtualAdapterName(std::wstring const& name);
        bool GetActiveNetworkSpeed(double& receiveBytesPerSec, double& sendBytesPerSec, double& receivePacketsPerSec, double& sendPacketsPerSec);

        winrt::Microsoft::UI::Xaml::DispatcherTimer clockTimer;
        winrt::event_token clockTickToken{};
        bool clockTickRegistered = false;

        inline static hstring greeting;
        inline static hstring username;
        inline static hstring hitokoto;
        inline static winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage avatar{ nullptr };

        // 性能显示
        std::unordered_map<int, DiskCardControl> disk_card_map;
        inline static hstring cpu_manufacture = L"", gpu_manufacture = L"", netadpt_manufacture = L"";
        inline static bool initialized, isNvidia, isNetSend = false;
        inline static double cache_l1, cache_l2, cache_l3;
        inline static bool net_selected = false;
        inline static DWORD active_net_if_index = 0;
        inline static UINT64 last_in_octets = 0, last_out_octets = 0, last_in_packets = 0, last_out_packets = 0;
        inline static ULONGLONG last_net_tick = 0;
        inline static nvmlDevice_t device;
        inline static PDH_HQUERY query;
        inline static PDH_HCOUNTER counter_cpu_time, counter_cpu_freq, counter_cpu_process, counter_cpu_thread, counter_cpu_syscall;
        inline static PDH_HCOUNTER counter_mem_cached, counter_mem_committed, counter_mem_read, counter_mem_write, counter_mem_input, counter_mem_output;
        inline static PDH_HCOUNTER counter_disk_time, counter_disk_trans, counter_disk_read, counter_disk_write, counter_disk_io;
        inline static PDH_HCOUNTER counter_gpu_time;

        void ChangeMode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}
namespace winrt::StarlightGUI::factory_implementation
{
    struct HomePage : public HomePageT<HomePage, implementation::HomePage>
    {
    };
}

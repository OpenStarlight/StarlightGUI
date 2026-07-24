#pragma once

#include "HomePage.g.h"
#include "Utils/Coroutine.h"
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

        std::unordered_map<int, DiskCardControl> diskCardMap;
        inline static hstring cpuManufacture = L"", gpuManufacture = L"", networkAdapterManufacture = L"";
        inline static bool initialized, isNvidia, isNetSend = false;
        inline static double cacheL1, cacheL2, cacheL3;
        inline static bool networkSelected = false;
        inline static DWORD activeNetworkInterfaceIndex = 0;
        inline static UINT64 lastInOctets = 0, lastOutOctets = 0, lastInPackets = 0, lastOutPackets = 0;
        inline static ULONGLONG lastNetworkTick = 0;
        inline static nvmlDevice_t device;
        inline static PDH_HQUERY query;
        inline static PDH_HCOUNTER cpuTimeCounter, cpuFrequencyCounter, processCounter, threadCounter, systemCallCounter;
        inline static PDH_HCOUNTER cachedMemoryCounter, committedMemoryCounter, memoryReadCounter, memoryWriteCounter, memoryInputCounter, memoryOutputCounter;
        inline static PDH_HCOUNTER diskTimeCounter, diskTransferCounter, diskReadCounter, diskWriteCounter, diskIoCounter;
        inline static PDH_HCOUNTER gpuTimeCounter;

        void ChangeMode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}
namespace winrt::StarlightGUI::factory_implementation
{
    struct HomePage : public HomePageT<HomePage, implementation::HomePage>
    {
    };
}

#include "pch.h"
#include "Process_ModulePage.xaml.h"
#if __has_include("Process_ModulePage.g.cpp")
#include "Process_ModulePage.g.cpp"
#endif


#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <sstream>
#include <iomanip>
#include <Utils/Utils.h>
#include <Utils/TaskUtils.h>
#include <Utils/KernelBase.h>
#include <InfoWindow.xaml.h>
#include <MainWindow.xaml.h>
#undef EnumProcessModules

using namespace winrt;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Imaging;
using namespace Windows::System;

namespace winrt::StarlightGUI::implementation
{
    Process_ModulePage::Process_ModulePage() {
        InitializeComponent();

        ModuleListView().ItemsSource(m_moduleList);
        ModuleListView().SizeChanged([weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::UpdateVisibleListViewMarqueeByNames(
                    self->ModuleListView(),
                    self->m_moduleList.Size(),
                    L"PrimaryTextContainer",
                    L"SecondaryTextBlock",
                    L"SecondaryMarquee");
            }
            });

        this->Loaded([this](auto&&, auto&&) {
            LoadModuleList();
            });

        LOG_INFO(L"Process_ModulePage", L"Process_ModulePage initialized.");
    }

    void Process_ModulePage::ModuleListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = sender.as<ListView>();

        slg::SelectItemOnRightTapped(listView, e);

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::MokuaiInfo>();

        auto flyoutStyles = slg::GetStyles();

        MenuFlyout menuFlyout;

        auto itemRefresh = slg::CreateMenuItem(flyoutStyles, L"\ue72c", GetLocalizedString(L"ProcModule_Refresh").c_str(), [this](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            LoadModuleList();
            co_return;
            });

        MenuFlyoutSeparator separatorR;

        // 选项1.1
        auto item1_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", GetLocalizedString(L"ProcModule_CopyInfo").c_str());
        auto item1_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", GetLocalizedString(L"ProcModule_Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub1);
        auto item1_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", GetLocalizedString(L"ProcModule_Path").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Path().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub2);
        auto item1_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", GetLocalizedString(L"ProcModule_Address").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Address().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub3);

        menuFlyout.Items().Append(itemRefresh);
        menuFlyout.Items().Append(separatorR);
        menuFlyout.Items().Append(item1_1);

        slg::ShowAt(menuFlyout, listView, e);
    }

    void Process_ModulePage::ModuleListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue()) return;

        auto itemContainer = args.ItemContainer().try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
        if (!itemContainer) return;

        auto contentRoot = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
        if (!contentRoot) return;

        slg::UpdateTextMarqueeByNames(
            contentRoot,
            L"PrimaryTextContainer",
            L"SecondaryTextBlock",
            L"SecondaryMarquee");

        DispatcherQueue().TryEnqueue([weak = get_weak(), contentRoot]() {
            if (auto self = weak.get()) {
                slg::UpdateTextMarqueeByNames(
                    contentRoot,
                    L"PrimaryTextContainer",
                    L"SecondaryTextBlock",
                    L"SecondaryMarquee");
            }
            });
    }

    winrt::Windows::Foundation::IAsyncAction Process_ModulePage::LoadModuleList()
    {
        if (!processForInfoWindow) co_return;
        // 跳过内核进程，获取可能导致异常或蓝屏
        if (processForInfoWindow.Name() == L"Idle" || processForInfoWindow.Name() == L"System" || processForInfoWindow.Name() == L"Registry" || processForInfoWindow.Name() == L"Memory Compression" || processForInfoWindow.Name() == L"Secure System" || processForInfoWindow.Name() == L"Unknown") {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"ProcModule_NoInfo").c_str(), InfoBarSeverity::Warning, g_infoWindowInstance);
            co_return;
        }

        LOG_INFO(__WFUNCTION__, L"Loading module list... (pid=%d)", processForInfoWindow.Id());
        m_moduleList.Clear();
        LoadingRing().IsActive(true);

        auto start = std::chrono::high_resolution_clock::now();

        auto lifetime = get_strong();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::MokuaiInfo> modules;
        modules.reserve(500);

        // 获取句柄列表
        KernelInstance::EnumProcessModules(processForInfoWindow.EProcessULong(), modules);
        LOG_INFO(__WFUNCTION__, L"Enumerated modules, %d entry(s).", modules.size());

        co_await wil::resume_foreground(DispatcherQueue());

        if (modules.size() >= 1000) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"ProcModule_TooManyModules").c_str(), InfoBarSeverity::Warning, g_infoWindowInstance);
        }

        for (const auto& module : modules) {
            if (module.Name().empty()) module.Name(GetLocalizedString(L"ProcModule_Unknown"));
            if (module.Path().empty()) module.Path(GetLocalizedString(L"ProcModule_Unknown"));

            m_moduleList.Append(module);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 更新模块数量文本
        std::wstringstream countText;
        countText << GetLocalizedString(L"ProcModule_CountPrefix").c_str() << L" " << m_moduleList.Size() << L" " << GetLocalizedString(L"ProcModule_CountSuffix").c_str() << L" (" << duration.count() << " ms)";
        ModuleCountText().Text(countText.str());
        LoadingRing().IsActive(false);
        slg::UpdateVisibleListViewMarqueeByNames(
            ModuleListView(),
            m_moduleList.Size(),
            L"PrimaryTextContainer",
            L"SecondaryTextBlock",
            L"SecondaryMarquee");

        LOG_INFO(__WFUNCTION__, L"Loaded module list, %d entry(s) in total.", m_moduleList.Size());
    }
}




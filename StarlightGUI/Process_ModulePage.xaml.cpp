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
        SetupLocalization();

        ModuleListView().ItemsSource(m_moduleList);
        HeaderColumnsGrid().LayoutUpdated([weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::SyncListViewColumnWidths(self->HeaderColumnsGrid(), self->BodyColumnsGrid(), self->ModuleListView(), 0);
            }
            });

        this->Loaded([this](auto&&, auto&&) {
            slg::SyncListViewColumnWidths(HeaderColumnsGrid(), BodyColumnsGrid(), ModuleListView(), 0);
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

        auto itemRefresh = slg::CreateMenuItem(flyoutStyles, L"\ue72c", t(L"Common.Refresh").c_str(), [this](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            LoadModuleList();
            co_return;
            });

        MenuFlyoutSeparator separatorR;

        // 选项1.1
        auto item1_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
        auto item1_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Common.Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub1);
        auto item1_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Path").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Path().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub2);
        auto item1_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"Common.Address").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Address().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_infoWindowInstance);
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

        slg::ApplyHeaderColumnWidthsToContainer(HeaderColumnsGrid(), itemContainer, 0);

    }

    winrt::Windows::Foundation::IAsyncAction Process_ModulePage::LoadModuleList()
    {
        if (!processForInfoWindow) co_return;
        // 跳过内核进程，获取可能导致异常或蓝屏
        if (processForInfoWindow.Name() == L"Idle" || processForInfoWindow.Name() == L"System" || processForInfoWindow.Name() == L"Registry" || processForInfoWindow.Name() == L"Memory Compression" || processForInfoWindow.Name() == L"Secure System" || processForInfoWindow.Name() == L"Unknown") {
            slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"ProcModule.Msg.NoInfo").c_str(), InfoBarSeverity::Warning, g_infoWindowInstance);
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
            slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"ProcModule.Msg.TooManyModules").c_str(), InfoBarSeverity::Warning, g_infoWindowInstance);
        }

        for (const auto& module : modules) {
            if (module.Name().empty()) module.Name(t(L"Common.Unknown"));
            if (module.Path().empty()) module.Path(t(L"Common.Unknown"));

            m_moduleList.Append(module);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 更新模块数量文本
        ModuleCountText().Text(t(L"ProcModule.Detail", static_cast<size_t>(m_moduleList.Size()), static_cast<long long>(duration.count())));
        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded module list, %d entry(s) in total.", m_moduleList.Size());
    }

    void Process_ModulePage::SetupLocalization()
    {
        ModuleTitleText().Text(t(L"ProcModule.Title"));
        ModuleCountText().Text(t(L"Msg.Loading"));
        ModuleNameHeaderButton().Content(tbox(L"Common.Name"));
        AddressHeaderButton().Content(tbox(L"Common.Address"));
        SizeHeaderButton().Content(tbox(L"Common.Size"));
	}
}






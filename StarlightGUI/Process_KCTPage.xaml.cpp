#include "pch.h"
#include "Process_KCTPage.xaml.h"
#if __has_include("Process_KCTPage.g.cpp")
#include "Process_KCTPage.g.cpp"
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
    Process_KCTPage::Process_KCTPage() {
        InitializeComponent();

        KCTListView().ItemsSource(m_kctList);
        if (!list_animation) KCTListView().ItemContainerTransitions().Clear();

        this->Loaded([this](auto&&, auto&&) {
            LoadKCTList();
            });

        LOG_INFO(L"Process_KCTPage", L"Process_KCTPage initialized.");
    }

    void Process_KCTPage::KCTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = sender.as<ListView>();

        slg::SelectItemOnRightTapped(listView, e);

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::KCTInfo>();

        auto flyoutStyles = slg::GetStyles();

        MenuFlyout menuFlyout;

        auto itemRefresh = slg::CreateMenuItem(flyoutStyles, L"\ue72c", L"刷新", [this](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            LoadKCTList();
            co_return;
            });

        MenuFlyoutSeparator separatorR;

        // 选项1.1
        auto item1_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", L"复制信息");
        auto item1_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", L"名称", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub1);
        auto item1_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", L"地址", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Address().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item1_1.Items().Append(item1_1_sub2);

        menuFlyout.Items().Append(itemRefresh);
        menuFlyout.Items().Append(separatorR);
        menuFlyout.Items().Append(item1_1);

        slg::ShowAt(menuFlyout, listView, e);
    }

    void Process_KCTPage::KCTListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue())
            return;

        // 将 Tag 设到容器上，便于 ListViewItemPresenter 通过 TemplatedParent 绑定
        if (auto itemContainer = args.ItemContainer())
            itemContainer.Tag(sender.Tag());
    }

    winrt::Windows::Foundation::IAsyncAction Process_KCTPage::LoadKCTList()
    {
        if (!processForInfoWindow) co_return;
        // 跳过内核进程，获取可能导致异常或蓝屏
        if (processForInfoWindow.Id() >= 0 && processForInfoWindow.Id() <= 272) {
            slg::CreateInfoBarAndDisplay(L"警告", L"该进程不包含任何此类型的信息！", InfoBarSeverity::Warning, g_infoWindowInstance);
            co_return;
        }

        LOG_INFO(__WFUNCTION__, L"Loading kernel callback table list... (pid=%d)", processForInfoWindow.Id());
        m_kctList.Clear();
        LoadingRing().IsActive(true);

        auto start = std::chrono::high_resolution_clock::now();

        auto lifetime = get_strong();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::KCTInfo> kcts;
        kcts.reserve(500);

        // 获取回调表
        KernelInstance::EnumProcessKernelCallbackTable(processForInfoWindow.EProcessULong(), kcts);
        LOG_INFO(__WFUNCTION__, L"Enumerated kernel callback tables, %d entry(s).", kcts.size());

        co_await wil::resume_foreground(DispatcherQueue());

        if (kcts.size() >= 1000) {
            slg::CreateInfoBarAndDisplay(L"警告", L"该进程持有过多内核回调表记录，程序无法完整显示，将显示前1000条！", InfoBarSeverity::Warning, g_infoWindowInstance);
        }

        for (const auto& kct : kcts) {
            if (kct.Name().empty()) kct.Name(L"(未知)");
            if (kct.Address().empty()) kct.Address(L"(未知)");

            m_kctList.Append(kct);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 更新模块数量文本
        std::wstringstream countText;
        countText << L"共 " << m_kctList.Size() << L" 个内核回调表记录 (" << duration.count() << " ms)";
        KCTCountText().Text(countText.str());
        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded kernel callback table list, %d entry(s) in total.", m_kctList.Size());
    }
}

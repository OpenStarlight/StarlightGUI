#include "pch.h"
#include "KernelModulePage.xaml.h"
#if __has_include("KernelModulePage.g.cpp")
#include "KernelModulePage.g.cpp"
#endif


#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <unordered_set>
#include <InfoWindow.xaml.h>
#include <MainWindow.xaml.h>
#include <LoadDriverDialog.xaml.h>

using namespace winrt;
using namespace WinUI3Package;
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
    static std::vector<winrt::StarlightGUI::KernelModuleInfo> fullRecordedKernelModules;
    static int safeAcceptedImage = -1;

    KernelModulePage::KernelModulePage() {
        InitializeComponent();

        KernelModuleListView().ItemsSource(m_kernelModuleList);
        if (!list_animation) KernelModuleListView().ItemContainerTransitions().Clear();

        this->Loaded([this](auto&&, auto&&) {
            LoadKernelModuleList();
            });

        LOG_INFO(L"KernelModulePage", L"KernelModulePage initialized.");
    }

    void KernelModulePage::KernelModuleListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = KernelModuleListView();

        slg::SelectItemOnRightTapped(listView, e);

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::KernelModuleInfo>();

        auto flyoutStyles = slg::GetStyles();

        if (item.Name() == L"AstralX.sys" || item.Name() == L"kernel.sys") {
            slg::CreateInfoBarAndDisplay(L"警告", L"你要干什么？", InfoBarSeverity::Warning, g_mainWindowInstance);
            return;
        }

        MenuFlyout menuFlyout;

        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\uec91", L"卸载", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::UnloadDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功卸载模块: " + item.Name(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法卸载模块: " + item.Name() + L", 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.2
        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ued1a", L"隐藏", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::HideDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功隐藏模块: " + item.Name(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法隐藏模块: " + item.Name() + L", 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", L"复制信息");
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", L"名称", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", L"路径", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Path().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);
        auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", L"基址", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.ImageBase().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub3);
        auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", L"驱动对象", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.DriverObject().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub4);

        menuFlyout.Items().Append(item1_1);
        menuFlyout.Items().Append(item1_2);
        menuFlyout.Items().Append(separator1);
        menuFlyout.Items().Append(item2_1);

        slg::ShowAt(menuFlyout, listView, e);
    }

    void KernelModulePage::KernelModuleListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue())
            return;

        // 将 Tag 设到容器上，便于 ListViewItemPresenter 通过 TemplatedParent 绑定
        if (auto itemContainer = args.ItemContainer())
            itemContainer.Tag(sender.Tag());
    }

    winrt::Windows::Foundation::IAsyncAction KernelModulePage::LoadKernelModuleList()
    {
        if (m_isLoadingKernelModules) {
            co_return;
        }
        m_isLoadingKernelModules = true;

        LOG_INFO(__WFUNCTION__, L"Loading kernel module list...");
        m_kernelModuleList.Clear();
        LoadingRing().IsActive(true);

        auto start = std::chrono::steady_clock::now();

        auto lifetime = get_strong();

        winrt::hstring query = KernelModuleSearchBox().Text();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::KernelModuleInfo> kernelModules;
        kernelModules.reserve(200);

        KernelInstance::EnumDrivers(kernelModules);
        LOG_INFO(__WFUNCTION__, L"Enumerated kernel modules, %d entry(s).", kernelModules.size());

        fullRecordedKernelModules = kernelModules;

        co_await wil::resume_foreground(DispatcherQueue());

        for (const auto& kernelModule : kernelModules) {
            bool shouldRemove = query.empty() ? false : ApplyFilter(kernelModule, query);
            if (shouldRemove) continue;

            if (kernelModule.Name().empty()) kernelModule.Name(L"(未知)");
            if (kernelModule.Path().empty()) kernelModule.Path(L"(未知)");
            if (kernelModule.ImageBase().empty()) kernelModule.ImageBase(L"(未知)");
            if (kernelModule.DriverObject().empty()) kernelModule.DriverObject(L"(未知)");
            if (kernelModule.DriverObjectULong() == 0x0) kernelModule.DriverObject(L"(无)");

            m_kernelModuleList.Append(kernelModule);
        }

        // 恢复排序
        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // 更新内核模块数量文本
        std::wstringstream countText;
        countText << L"共 " << m_kernelModuleList.Size() << L" 个内核模块 (" << duration << " ms)";
        KernelModuleCountText().Text(countText.str());

        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded kernel module list, %d entry(s) in total.", m_kernelModuleList.Size());
        m_isLoadingKernelModules = false;
    }

    void KernelModulePage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        if (columnName == L"Name")
        {
            ApplySort(m_isNameAscending, "Name");
        }
        else if (columnName == L"Size")
        {
            ApplySort(m_isSizeAscending, "Size");
        }
        else if (columnName == L"LoadOrder")
        {
            ApplySort(m_isLoadOrderAscending, "LoadOrder");
        }
    }

    // 排序切换
    slg::coroutine KernelModulePage::ApplySort(bool& isAscending, const std::string& column)
    {
        NameHeaderButton().Content(box_value(L"模块"));
        SizeHeaderButton().Content(box_value(L"大小"));
        LoadOrderHeaderButton().Content(box_value(L"加载顺序"));

        std::vector<winrt::StarlightGUI::KernelModuleInfo> sortedKernelModules;

        for (auto const& kernelModule : m_kernelModuleList) {
            sortedKernelModules.push_back(kernelModule);
        }

        if (column == "Name") {
            if (isAscending) {
                NameHeaderButton().Content(box_value(L"模块 ↓"));
                std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName < bName;
                    });

            }
            else {
                NameHeaderButton().Content(box_value(L"模块 ↑"));
                std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName > bName;
                    });
            }
        }
        else if (column == "Size") {
            if (isAscending) {
                SizeHeaderButton().Content(box_value(L"大小 ↓"));
                std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [](auto a, auto b) {
                    return a.SizeULong() < b.SizeULong();
                    });
            }
            else {
                SizeHeaderButton().Content(box_value(L"大小 ↑"));
                std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [](auto a, auto b) {
                    return a.SizeULong() > b.SizeULong();
                    });
            }
        }
        else if (column == "LoadOrder") {
            if (isAscending) {
                LoadOrderHeaderButton().Content(box_value(L"加载顺序 ↓"));
                std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [](auto a, auto b) {
                    return a.LoadOrderULong() < b.LoadOrderULong();
                    });
            }
            else {
                LoadOrderHeaderButton().Content(box_value(L"加载顺序 ↑"));
                std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [](auto a, auto b) {
                    return a.LoadOrderULong() > b.LoadOrderULong();
                    });
            }
        }

        m_kernelModuleList.Clear();
        for (auto& kernelModule : sortedKernelModules) {
            m_kernelModuleList.Append(kernelModule);
        }

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    void KernelModulePage::KernelModuleSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;

        WaitAndReloadAsync(200);
    }

    bool KernelModulePage::ApplyFilter(const winrt::StarlightGUI::KernelModuleInfo& kernelModule, hstring& query) {
        std::wstring name = kernelModule.Name().c_str();
        std::wstring queryWStr = query.c_str();

        // 不比较大小写
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(queryWStr.begin(), queryWStr.end(), queryWStr.begin(), ::towlower);

        bool result = name.find(queryWStr) == std::wstring::npos;

        return result;
    }


    slg::coroutine KernelModulePage::RefreshKernelModuleListButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        RefreshKernelModuleListButton().IsEnabled(false);
        co_await LoadKernelModuleList();
        RefreshKernelModuleListButton().IsEnabled(true);
        co_return;
    }

    slg::coroutine KernelModulePage::LoadDriverButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        try {
            auto dialog = winrt::make<winrt::StarlightGUI::implementation::LoadDriverDialog>();
            dialog.XamlRoot(this->XamlRoot());

            auto result = co_await dialog.ShowAsync();

            if (result == ContentDialogResult::Primary) {
                hstring driverPath = dialog.DriverPath();
                bool bypass = dialog.Bypass();

                auto file = co_await winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(driverPath);

                if (bypass) {
                    LOG_WARNING(__WFUNCTION__, L"Bypass flag enabled! Disabling DSE...");
                    KernelInstance::DisableDSE();
                }

                bool status = DriverUtils::LoadDriver(driverPath.c_str(), file.Name().c_str(), unused);

                if (bypass) {
                    KernelInstance::EnableDSE();
                }

                if (status) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"驱动加载成功！", InfoBarSeverity::Success, g_mainWindowInstance);
                }
                else {
                    slg::CreateInfoBarAndDisplay(L"失败", L"驱动加载失败, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
                }

                LoadKernelModuleList();
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(L"错误", L"显示对话框失败: " + ex.message(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine KernelModulePage::UnloadModuleButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (KernelModuleListView().SelectedItem()) {
            auto item = KernelModuleListView().SelectedItem().as<winrt::StarlightGUI::KernelModuleInfo>();

            if (item.Name() == L"AstralX.sys" || item.Name() == L"kernel.sys") {
                slg::CreateInfoBarAndDisplay(L"警告", L"你要干什么？", InfoBarSeverity::Warning, g_mainWindowInstance);
                co_return;
            }

            if (KernelInstance::UnloadDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功卸载模块: " + item.Name(), InfoBarSeverity::Success, g_mainWindowInstance);

                LoadKernelModuleList();
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法卸载模块: " + item.Name() + L", 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction KernelModulePage::WaitAndReloadAsync(int interval) {
        auto lifetime = get_strong();

        reloadTimer.Stop();
        reloadTimer.Interval(std::chrono::milliseconds(interval));
        reloadTimer.Tick([this](auto&&, auto&&) {
            LoadKernelModuleList();
            reloadTimer.Stop();
            });
        reloadTimer.Start();

        co_return;
    }
}

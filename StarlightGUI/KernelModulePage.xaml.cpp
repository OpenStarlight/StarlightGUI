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
#include <array>
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
        KernelModuleListView().SizeChanged([weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::UpdateVisibleListViewMarqueeByNames(
                    self->KernelModuleListView(),
                    self->m_kernelModuleList.Size(),
                    L"PrimaryTextContainer",
                    L"SecondaryTextBlock",
                    L"SecondaryMarquee");
            }
            });

        this->Loaded([this](auto&&, auto&&) {
            LoadKernelModuleList();
            });
        this->Unloaded([this](auto&&, auto&&) {
            ++m_reloadRequestVersion;
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
        slg::UpdateVisibleListViewMarqueeByNames(
            KernelModuleListView(),
            m_kernelModuleList.Size(),
            L"PrimaryTextContainer",
            L"SecondaryTextBlock",
            L"SecondaryMarquee");

        LOG_INFO(__WFUNCTION__, L"Loaded kernel module list, %d entry(s) in total.", m_kernelModuleList.Size());
        m_isLoadingKernelModules = false;
    }

    void KernelModulePage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        struct SortBinding {
            wchar_t const* tag;
            char const* column;
            bool* ascending;
        };

        static const std::array<SortBinding, 5> bindings{ {
            { L"Name", "Name", &KernelModulePage::m_isNameAscending },
            { L"ImageBase", "ImageBase", &KernelModulePage::m_isImageBaseAscending },
            { L"DriverObject", "DriverObject", &KernelModulePage::m_isDriverObjectAscending },
            { L"Size", "Size", &KernelModulePage::m_isSizeAscending },
            { L"LoadOrder", "LoadOrder", &KernelModulePage::m_isLoadOrderAscending },
        } };

        for (auto const& binding : bindings) {
            if (columnName == binding.tag) {
                ApplySort(*binding.ascending, binding.column);
                break;
            }
        }
    }

    // 排序切换
    slg::coroutine KernelModulePage::ApplySort(bool& isAscending, const std::string& column)
    {
        SortKernelModuleList(isAscending, column, true);

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    void KernelModulePage::SortKernelModuleList(bool isAscending, const std::string& column, bool updateHeader)
    {
        if (column.empty()) return;

        enum class SortColumn {
            Unknown,
            Name,
            ImageBase,
            DriverObject,
            Size,
            LoadOrder
        };

        auto resolveSortColumn = [&](const std::string& key) -> SortColumn {
            if (key == "Name") return SortColumn::Name;
            if (key == "ImageBase") return SortColumn::ImageBase;
            if (key == "DriverObject") return SortColumn::DriverObject;
            if (key == "Size") return SortColumn::Size;
            if (key == "LoadOrder") return SortColumn::LoadOrder;
            return SortColumn::Unknown;
            };

        auto activeColumn = resolveSortColumn(column);
        if (activeColumn == SortColumn::Unknown) return;

        if (updateHeader) {
            NameHeaderButton().Content(box_value(L"模块"));
            ImageBaseHeaderButton().Content(box_value(L"基址"));
            DriverObjectHeaderButton().Content(box_value(L"驱动对象"));
            SizeHeaderButton().Content(box_value(L"大小"));
            LoadOrderHeaderButton().Content(box_value(L"加载顺序"));

            if (activeColumn == SortColumn::Name) NameHeaderButton().Content(box_value(isAscending ? L"模块 ↓" : L"模块 ↑"));
            if (activeColumn == SortColumn::ImageBase) ImageBaseHeaderButton().Content(box_value(isAscending ? L"基址 ↓" : L"基址 ↑"));
            if (activeColumn == SortColumn::DriverObject) DriverObjectHeaderButton().Content(box_value(isAscending ? L"驱动对象 ↓" : L"驱动对象 ↑"));
            if (activeColumn == SortColumn::Size) SizeHeaderButton().Content(box_value(isAscending ? L"大小 ↓" : L"大小 ↑"));
            if (activeColumn == SortColumn::LoadOrder) LoadOrderHeaderButton().Content(box_value(isAscending ? L"加载顺序 ↓" : L"加载顺序 ↑"));
        }

        std::vector<winrt::StarlightGUI::KernelModuleInfo> sortedKernelModules;
        sortedKernelModules.reserve(m_kernelModuleList.Size());
        for (auto const& kernelModule : m_kernelModuleList) {
            sortedKernelModules.push_back(kernelModule);
        }

        auto sortActiveColumn = [&](const winrt::StarlightGUI::KernelModuleInfo& a, const winrt::StarlightGUI::KernelModuleInfo& b) -> bool {
            switch (activeColumn) {
            case SortColumn::Name:
                return LessIgnoreCase(a.Name().c_str(), b.Name().c_str());
            case SortColumn::ImageBase:
                return a.ImageBaseULong() < b.ImageBaseULong();
            case SortColumn::DriverObject:
                return a.DriverObjectULong() < b.DriverObjectULong();
            case SortColumn::Size:
                return a.SizeULong() < b.SizeULong();
            case SortColumn::LoadOrder:
                return a.LoadOrderULong() < b.LoadOrderULong();
            default:
                return false;
            }
            };

        if (isAscending) {
            std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), sortActiveColumn);
        }
        else {
            std::sort(sortedKernelModules.begin(), sortedKernelModules.end(), [&](const auto& a, const auto& b) {
                return sortActiveColumn(b, a);
                });
        }

        m_kernelModuleList.Clear();
        for (auto& kernelModule : sortedKernelModules) {
            m_kernelModuleList.Append(kernelModule);
        }
    }

    void KernelModulePage::KernelModuleSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;

        WaitAndReloadAsync(250);
    }

    bool KernelModulePage::ApplyFilter(const winrt::StarlightGUI::KernelModuleInfo& kernelModule, hstring& query) {
        return !ContainsIgnoreCase(kernelModule.Name().c_str(), query.c_str());
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
        auto requestVersion = ++m_reloadRequestVersion;

        co_await winrt::resume_after(std::chrono::milliseconds(interval));
        co_await wil::resume_foreground(DispatcherQueue());

        if (!IsLoaded() || requestVersion != m_reloadRequestVersion) co_return;
        LoadKernelModuleList();

        co_return;
    }
}



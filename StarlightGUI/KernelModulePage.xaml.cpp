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

        KernelModuleTitleUid().Text(GetLocalizedString(L"KernelModule_Title.Text"));
        KernelModuleCountText().Text(GetLocalizedString(L"KernelModule_Loading.Text"));
        RefreshKernelModuleListButton().Label(GetLocalizedString(L"KernelModule_Refresh.Label"));
        LoadDriverButton().Label(GetLocalizedString(L"KernelModule_LoadDriver.Label"));
        UnloadModuleButton().Label(GetLocalizedString(L"KernelModule_UnloadModule.Label"));
        KernelModuleSearchBox().PlaceholderText(GetLocalizedString(L"KernelModule_SearchBox.PlaceholderText"));
        NameHeaderButton().Content(tbox(L"KernelModule_ColModule.Content"));
        ImageBaseHeaderButton().Content(tbox(L"KernelModule_ColBase.Content"));
        DriverObjectHeaderButton().Content(tbox(L"KernelModule_ColDriverObj.Content"));
        SizeHeaderButton().Content(tbox(L"KernelModule_ColSize.Content"));
        LoadOrderHeaderButton().Content(tbox(L"KernelModule_ColLoadOrder.Content"));

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
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Msg_WhatAreYouDoing").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            return;
        }

        MenuFlyout menuFlyout;

        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\uec91", GetLocalizedString(L"KModule_Unload").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::UnloadDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"KModule_UnloadSuccess") + item.Name()).c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"KModule_UnloadFailed") + item.Name() + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.2
        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ued1a", GetLocalizedString(L"KModule_Hide").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::HideDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"KModule_HideSuccess") + item.Name()).c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"KModule_HideFailed") + item.Name() + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", GetLocalizedString(L"KModule_CopyInfo").c_str());
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", GetLocalizedString(L"KModule_Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", GetLocalizedString(L"KModule_Path").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Path().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);
        auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", GetLocalizedString(L"KModule_Base").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.ImageBase().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub3);
        auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", GetLocalizedString(L"KModule_DriverObj").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.DriverObject().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
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
        std::wstring lowerQuery;
        if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());

        co_await wil::resume_foreground(DispatcherQueue());

        for (const auto& kernelModule : kernelModules) {
            bool shouldRemove = lowerQuery.empty() ? false : !ContainsIgnoreCaseLowerQuery(kernelModule.Name().c_str(), lowerQuery);
            if (shouldRemove) continue;

            if (kernelModule.Name().empty()) kernelModule.Name(GetLocalizedString(L"Msg_Unknown"));
            if (kernelModule.Path().empty()) kernelModule.Path(GetLocalizedString(L"Msg_Unknown"));
            if (kernelModule.ImageBase().empty()) kernelModule.ImageBase(GetLocalizedString(L"Msg_Unknown"));
            if (kernelModule.DriverObject().empty()) kernelModule.DriverObject(GetLocalizedString(L"Msg_Unknown"));
            if (kernelModule.DriverObjectULong() == 0x0) kernelModule.DriverObject(GetLocalizedString(L"Msg_None"));

            m_kernelModuleList.Append(kernelModule);
        }

        // 恢复排序
        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // 更新内核模块数量文本
        std::wstringstream countText;
        countText << m_kernelModuleList.Size() << L" (" << duration << " ms)";
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
            NameHeaderButton().Content(tbox(L"KModule_ColModule_Text"));
            ImageBaseHeaderButton().Content(tbox(L"KModule_ColBase_Text"));
            DriverObjectHeaderButton().Content(tbox(L"KModule_ColDriverObj_Text"));
            SizeHeaderButton().Content(tbox(L"KModule_ColSize_Text"));
            LoadOrderHeaderButton().Content(tbox(L"KModule_ColLoadOrder_Text"));

            if (activeColumn == SortColumn::Name) NameHeaderButton().Content(isAscending ? tbox(L"KModule_ColModule_Down") : tbox(L"KModule_ColModule_Up"));
            if (activeColumn == SortColumn::ImageBase) ImageBaseHeaderButton().Content(isAscending ? tbox(L"KModule_ColBase_Down") : tbox(L"KModule_ColBase_Up"));
            if (activeColumn == SortColumn::DriverObject) DriverObjectHeaderButton().Content(isAscending ? tbox(L"KModule_ColDriverObj_Down") : tbox(L"KModule_ColDriverObj_Up"));
            if (activeColumn == SortColumn::Size) SizeHeaderButton().Content(isAscending ? tbox(L"KModule_ColSize_Down") : tbox(L"KModule_ColSize_Up"));
            if (activeColumn == SortColumn::LoadOrder) LoadOrderHeaderButton().Content(isAscending ? tbox(L"KModule_ColLoadOrder_Down") : tbox(L"KModule_ColLoadOrder_Up"));
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
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"KModule_LoadDriverSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                }
                else {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"KModule_LoadDriverFailed") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                }

                LoadKernelModuleList();
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), (GetLocalizedString(L"KModule_ShowDialogFailed") + ex.message()).c_str(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine KernelModulePage::UnloadModuleButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (KernelModuleListView().SelectedItem()) {
            auto item = KernelModuleListView().SelectedItem().as<winrt::StarlightGUI::KernelModuleInfo>();

            if (item.Name() == L"AstralX.sys" || item.Name() == L"kernel.sys") {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Msg_WhatAreYouDoing").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
                co_return;
            }

            if (KernelInstance::UnloadDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"KModule_UnloadSuccess") + item.Name()).c_str(), InfoBarSeverity::Success, g_mainWindowInstance);

                LoadKernelModuleList();
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"KModule_UnloadFailed") + item.Name() + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
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







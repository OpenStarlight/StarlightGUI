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
        SetupLocalization();

        KernelModuleListView().ItemsSource(m_kernelModuleList);
        HeaderColumnsGrid().LayoutUpdated([weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::SyncListViewColumnWidths(
                    self->HeaderColumnsGrid(),
                    self->BodyColumnsGrid(),
                    self->KernelModuleListView(),
                    0);
            }
            });

        this->Loaded([this](auto&&, auto&&) {
            slg::SyncListViewColumnWidths(HeaderColumnsGrid(), BodyColumnsGrid(), KernelModuleListView(), 0);
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
            slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"Msg.EditSelfWarning").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            return;
        }

        MenuFlyout menuFlyout;

        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\uec91", t(L"KernelModule.Menu.Unload").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::UnloadDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.2
        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ued1a", t(L"KernelModule.Menu.Hide").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::HideDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Common.Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Path").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Path().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);
        auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Common.Base").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.ImageBase().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub3);
        auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"KernelModule.Header.DriverObj").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.DriverObject().c_str())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
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

        slg::ApplyHeaderColumnWidthsToContainer(HeaderColumnsGrid(), itemContainer, 0);

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

            if (kernelModule.Name().empty()) kernelModule.Name(t(L"Common.Unknown"));
            if (kernelModule.Path().empty()) kernelModule.Path(t(L"Common.Unknown"));
            if (kernelModule.ImageBase().empty()) kernelModule.ImageBase(t(L"Common.Unknown"));
            if (kernelModule.DriverObject().empty()) kernelModule.DriverObject(t(L"Common.Unknown"));
            if (kernelModule.DriverObjectULong() == 0x0) kernelModule.DriverObject(t(L"Common.None"));

            m_kernelModuleList.Append(kernelModule);
        }

        // 恢复排序
        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // 更新内核模块数量文本
        KernelModuleCountText().Text(t(L"KernelModule.Detail", m_kernelModuleList.Size(), duration));

        LoadingRing().IsActive(false);

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
            { L"Index", "Index", &KernelModulePage::m_isIndexAscending },
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
            Index
        };

        auto resolveSortColumn = [&](const std::string& key) -> SortColumn {
            if (key == "Name") return SortColumn::Name;
            if (key == "ImageBase") return SortColumn::ImageBase;
            if (key == "DriverObject") return SortColumn::DriverObject;
            if (key == "Size") return SortColumn::Size;
            if (key == "Index") return SortColumn::Index;
            return SortColumn::Unknown;
            };

        auto activeColumn = resolveSortColumn(column);
        if (activeColumn == SortColumn::Unknown) return;

        if (updateHeader) {
            NameHeaderButton().Content(tbox(L"Common.Module"));
            ImageBaseHeaderButton().Content(tbox(L"Common.Base"));
            DriverObjectHeaderButton().Content(tbox(L"KernelModule.Header.DriverObj"));
            SizeHeaderButton().Content(tbox(L"Common.Size"));
            IndexHeaderButton().Content(tbox(L"Common.Index"));

            if (activeColumn == SortColumn::Name) NameHeaderButton().Content(box_value(t(L"Common.Module") + (isAscending ? L" ↓" : L" ↑")));
            if (activeColumn == SortColumn::ImageBase) ImageBaseHeaderButton().Content(box_value(t(L"Common.Base") + (isAscending ? L" ↓" : L" ↑")));
            if (activeColumn == SortColumn::DriverObject) DriverObjectHeaderButton().Content(box_value(t(L"KernelModule.Header.DriverObj") + (isAscending ? L" ↓" : L" ↑")));
            if (activeColumn == SortColumn::Size) SizeHeaderButton().Content(box_value(t(L"Common.Size") + (isAscending ? L" ↓" : L" ↑")));
            if (activeColumn == SortColumn::Index) IndexHeaderButton().Content(box_value(t(L"Common.Index") + (isAscending ? L" ↓" : L" ↑")));
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
            case SortColumn::Index:
                return a.Index() < b.Index();
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

    void KernelModulePage::KernelModuleSearchBox_TextChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;

        auto searchBox = sender.try_as<AutoSuggestBox>();
        if (!searchBox) return;

        if (e.Reason() == AutoSuggestionBoxTextChangeReason::UserInput) {
            std::unordered_set<std::wstring> seen;
            auto suggestions = winrt::single_threaded_observable_vector<winrt::Windows::Foundation::IInspectable>();
            std::wstring lowerQuery = ToLowerCase(searchBox.Text().c_str());

            for (auto const& kernelModule : m_kernelModuleList) {
                std::wstring name = kernelModule.Name().c_str();
                if (name.empty()) continue;

                std::wstring lowerName = ToLowerCase(name);
                if (!lowerQuery.empty() && lowerName.find(lowerQuery) == std::wstring::npos) continue;
                if (!seen.insert(lowerName).second) continue;

                suggestions.Append(box_value(kernelModule.Name()));
                if (suggestions.Size() >= 20) break;
            }

            searchBox.ItemsSource(suggestions);
        }

        WaitAndReloadAsync(250);
    }

    void KernelModulePage::KernelModuleSearchBox_SuggestionChosen(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e)
    {
        auto selected = e.SelectedItem().try_as<winrt::Windows::Foundation::IReference<winrt::hstring>>();
        hstring target = selected ? selected.Value() : unbox_value<hstring>(e.SelectedItem());
        if (target.empty()) return;

        KernelModuleSearchBox().Text(target);
    }

    void KernelModulePage::KernelModuleSearchBox_QuerySubmitted(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e)
    {
        (void)e;
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

                bool status = DriverUtils::LoadDriver(driverPath.c_str(), file.Name().c_str());

                if (bypass) {
                    KernelInstance::EnableDSE();
                }

                if (status) {
                    slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
                }
                else {
                    slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
                }

                LoadKernelModuleList();
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(t(L"Common.Error"), (t(L"Msg.ShowDialog.Failed") + ex.message()).c_str(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine KernelModulePage::UnloadModuleButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (KernelModuleListView().SelectedItem()) {
            auto item = KernelModuleListView().SelectedItem().as<winrt::StarlightGUI::KernelModuleInfo>();

            if (item.Name() == L"AstralX.sys" || item.Name() == L"kernel.sys") {
                slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"Msg.EditSelfWarning").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
                co_return;
            }

            if (KernelInstance::UnloadDriver(item.DriverObjectULong())) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);

                LoadKernelModuleList();
            }
            else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
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

    void KernelModulePage::SetupLocalization() {
        KernelModuleTitleUid().Text(t(L"KernelModule.Title"));
        KernelModuleCountText().Text(t(L"KernelModule.Loading"));
        RefreshKernelModuleListButton().Label(t(L"Common.Refresh"));
        LoadDriverButton().Label(t(L"KernelModule.Button.LoadDriver"));
        UnloadModuleButton().Label(t(L"KernelModule.Button.UnloadModule"));
        KernelModuleSearchBox().PlaceholderText(t(L"KernelModule.Placeholder"));
        NameHeaderButton().Content(tbox(L"Common.Module"));
        ImageBaseHeaderButton().Content(tbox(L"Common.Base"));
        DriverObjectHeaderButton().Content(tbox(L"KernelModule.Header.DriverObj"));
        SizeHeaderButton().Content(tbox(L"Common.Size"));
        IndexHeaderButton().Content(tbox(L"Common.Index"));
    }
}
#pragma once

#include "pch.h"
#include "MainWindow.xaml.h"
#include "InfoWindow.xaml.h"
#include <coroutine>
#include <exception>
#include <unordered_map>
#include <vector>
#include <dwmapi.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Media::Animation;

namespace slg {
    struct coroutine {
        coroutine();

        struct promise_type {
            coroutine get_return_object() const noexcept;

            void return_void() const noexcept;

            std::suspend_never initial_suspend() const noexcept;

            std::suspend_never final_suspend() const noexcept;

            void unhandled_exception() const noexcept;
        };
    };

    struct Styles
    {
        winrt::Microsoft::UI::Xaml::Style Item;
        winrt::Microsoft::UI::Xaml::Style SubItem;
    };

    Styles GetStyles();

    MenuFlyoutItem CreateMenuItem(
        Styles const& styles,
        hstring const& glyph,
        hstring const& text,
        winrt::Microsoft::UI::Xaml::RoutedEventHandler const& click = nullptr);

    MenuFlyoutItem CreateMenuItem(
        Styles const& styles,
        hstring const& text,
        winrt::Microsoft::UI::Xaml::RoutedEventHandler const& click = nullptr);

    MenuFlyoutSubItem CreateMenuSubItem(
        Styles const& styles,
        hstring const& glyph,
        hstring const& text);

    MenuFlyoutSubItem CreateMenuSubItem(
        Styles const& styles,
        hstring const& text);

    void ShowAt(
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyout const& flyout,
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);

    FontIcon CreateFontIcon(hstring glyph);

    InfoBar CreateInfoBar(hstring title, hstring message, InfoBarSeverity severity, XamlRoot xamlRoot);

    void DisplayInfoBar(InfoBar infobar, Panel parent, int time = 1500);

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, XamlRoot xamlRoot, Panel parent, int time = 1500);

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, winrt::StarlightGUI::implementation::MainWindow* instance, int time = 1500);

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, winrt::StarlightGUI::implementation::InfoWindow* instance, int time = 1500);

    ContentDialog CreateContentDialog(hstring title, hstring content, hstring closeMessage, XamlRoot xamlRoot);

    DataTemplate GetContentDialogSuccessTemplate();

    DataTemplate GetContentDialogErrorTemplate();

    DataTemplate GetContentDialogInfoTemplate();

    DataTemplate GetTemplate(hstring xaml);

    bool CheckIllegalComboBoxAction(IInspectable const& sender, SelectionChangedEventArgs const& e);
    Microsoft::UI::Xaml::ElementTheme GetConfiguredElementTheme();
    void ApplyConfiguredTheme();

    winrt::Microsoft::UI::Xaml::Media::ImageSource CreateImageSourceFromHIcon(HICON hIcon, int iconSize = 16, bool destroyIcon = false);

    std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Media::ImageSource>& GetShellIconCacheStore();

    winrt::Microsoft::UI::Xaml::Media::ImageSource GetShellIconImage(
        std::wstring const& path,
        bool isDirectory,
        int iconSize = 16,
        bool useFileAttributes = false,
        std::wstring const& cacheKey = L"");

    void ClearShellIconCache();

    void ApplyHeaderColumnWidthsToRow(
        winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid,
        winrt::Microsoft::UI::Xaml::Controls::Grid const& rowGrid,
        uint32_t rowOffset = 0);

    void ApplyHeaderColumnWidthsToContainer(
        winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid,
        winrt::Microsoft::UI::Xaml::Controls::ListViewItem const& itemContainer,
        uint32_t rowOffset = 0);

    void SyncListViewColumnWidths(
        winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid,
        winrt::Microsoft::UI::Xaml::Controls::Grid const& bodyGrid,
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        uint32_t rowOffset = 0,
        double epsilon = 0.5);

    template <typename T>
    T FindParent(winrt::Microsoft::UI::Xaml::DependencyObject const& child)
    {
        auto parent = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetParent(child);
        while (parent && !parent.try_as<T>())
        {
            parent = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetParent(parent);
        }
        return parent.try_as<T>();
    }

    template <typename T>
    T FindVisualChild(winrt::Microsoft::UI::Xaml::DependencyObject const& parent)
    {
        if (!parent) return nullptr;

        int count = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(parent);
        for (int i = 0; i < count; ++i) {
            auto child = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChild(parent, i);
            if (auto typed = child.try_as<T>()) return typed;
            if (auto nested = FindVisualChild<T>(child)) return nested;
        }
        return nullptr;
    }

    inline bool SelectItemOnRightTapped(
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e,
        bool keepMultiSelection = false)
    {
        if (auto fe = e.OriginalSource().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>())
        {
            auto container = FindParent<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>(fe);
            if (container)
            {
                if (!keepMultiSelection || listView.SelectedItems().Size() < 2)
                {
                    listView.SelectedItem(container.Content());
                    return true;
                }
            }
        }
        return false;
    }

}

#pragma once

#include "WindowPage.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct WindowPage : WindowPageT<WindowPage>
    {
        WindowPage();

        slg::coroutine RefreshButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void WindowListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void WindowListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);
        void CheckBox_Checked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void ColumnHeader_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine ApplySort(bool& isAscending, const std::string& column);
        void SortWindowList(bool isAscending, const std::string& column, bool updateHeader);

        void SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        bool ApplyFilter(const winrt::StarlightGUI::WindowInfo& window, hstring& query);

        winrt::Windows::Foundation::IAsyncAction LoadWindowList();
        winrt::Windows::Foundation::IAsyncAction GetWindowInfoAsync(std::vector<winrt::StarlightGUI::WindowInfo>& windows);
        winrt::Windows::Foundation::IAsyncAction GetWindowIconAsync(const winrt::StarlightGUI::WindowInfo& window);
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);

        bool SetWindowZBID(HWND hwnd, ZBID zbid);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::WindowInfo> m_windowList{
            winrt::multi_threaded_observable_vector<winrt::StarlightGUI::WindowInfo>()
        };

        bool m_isLoadingWindows = false;
        uint64_t m_reloadRequestVersion = 0;

        inline static bool m_showVisibleOnly, m_showNoTitle = false;
        inline static bool m_isLoading = false;
        inline static bool m_isNameAscending = true;
        inline static bool m_isBandAscending = true;
        inline static bool m_isWindowStyleAscending = true;
        inline static bool m_isHwndAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct WindowPage : WindowPageT<WindowPage, implementation::WindowPage>
    {
    };
}

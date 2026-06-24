#pragma once

#include "App.xaml.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        void InitializeLogger();
        bool InitializeDriverBeforeWindow();

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}

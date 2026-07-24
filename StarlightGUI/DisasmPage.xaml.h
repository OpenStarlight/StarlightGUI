#pragma once

#include "DisasmPage.g.h"
#include "Utils/Coroutine.h"

namespace winrt::StarlightGUI::implementation
{
    struct DisasmPage : DisasmPageT<DisasmPage>
    {
        DisasmPage();
        void SetupLocalization();

        slg::coroutine Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct DisasmPage : DisasmPageT<DisasmPage, implementation::DisasmPage>
    {
    };
}

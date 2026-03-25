#pragma once

#include <string_view>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>

namespace winrt::StarlightGUI::implementation {
    winrt::hstring GetLocalizedString(const wchar_t* key) noexcept;

    winrt::hstring t(const wchar_t* key) noexcept;
    winrt::hstring t(std::wstring_view key) noexcept;
    winrt::hstring t(const char* key) noexcept;
    winrt::hstring t(std::string_view key) noexcept;

    winrt::Windows::Foundation::IInspectable GetLocalizedInspectable(const wchar_t* key) noexcept;

    winrt::Windows::Foundation::IInspectable tbox(const wchar_t* key) noexcept;
    winrt::Windows::Foundation::IInspectable tbox(std::wstring_view key) noexcept;
    winrt::Windows::Foundation::IInspectable tbox(const char* key) noexcept;
    winrt::Windows::Foundation::IInspectable tbox(std::string_view key) noexcept;
}

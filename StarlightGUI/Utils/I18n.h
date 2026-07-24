#pragma once

#include <cwchar>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>

namespace winrt::StarlightGUI::implementation {
    winrt::hstring GetLocalizedString(const wchar_t* key) noexcept;

    winrt::hstring t(const wchar_t* key) noexcept;
    winrt::hstring t(std::wstring_view key) noexcept;
    winrt::hstring t(const char* key) noexcept;
    winrt::hstring t(std::string_view key) noexcept;
    template<typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
    winrt::hstring t(const wchar_t* key, Args&&... args) noexcept {
        auto format = GetLocalizedString(key);
        int length = _scwprintf(format.c_str(), std::forward<Args>(args)...);
        if (length <= 0) return format;

        std::vector<wchar_t> buffer((size_t)length + 1);
        int written = swprintf_s(buffer.data(), buffer.size(), format.c_str(), std::forward<Args>(args)...);
        if (written <= 0) return format;

        return winrt::hstring(buffer.data());
    }

    winrt::Windows::Foundation::IInspectable GetLocalizedInspectable(const wchar_t* key) noexcept;

    winrt::Windows::Foundation::IInspectable tbox(const wchar_t* key) noexcept;
    winrt::Windows::Foundation::IInspectable tbox(std::wstring_view key) noexcept;
    winrt::Windows::Foundation::IInspectable tbox(const char* key) noexcept;
    winrt::Windows::Foundation::IInspectable tbox(std::string_view key) noexcept;
}

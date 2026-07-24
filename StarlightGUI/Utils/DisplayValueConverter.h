#pragma once

#include "DisplayValueConverter.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct DisplayValueConverter : DisplayValueConverterT<DisplayValueConverter>
	{
		DisplayValueConverter() = default;

		winrt::Windows::Foundation::IInspectable Convert(
			winrt::Windows::Foundation::IInspectable const& value,
			winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
			winrt::Windows::Foundation::IInspectable const& parameter,
			winrt::hstring const& language);

		winrt::Windows::Foundation::IInspectable ConvertBack(
			winrt::Windows::Foundation::IInspectable const& value,
			winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
			winrt::Windows::Foundation::IInspectable const& parameter,
			winrt::hstring const& language);
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct DisplayValueConverter : DisplayValueConverterT<DisplayValueConverter, implementation::DisplayValueConverter>
	{
	};
}

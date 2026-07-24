#include "pch.h"
#include "DisplayValueConverter.h"
#if __has_include("DisplayValueConverter.g.cpp")
#include "DisplayValueConverter.g.cpp"
#endif
#include "CppUtils.h"
#include "I18n.h"
#include "../SiriusIO.h"
#include <string_view>

namespace winrt::StarlightGUI::implementation
{
	winrt::Windows::Foundation::IInspectable DisplayValueConverter::Convert(
		winrt::Windows::Foundation::IInspectable const& value,
		winrt::Windows::UI::Xaml::Interop::TypeName const&,
		winrt::Windows::Foundation::IInspectable const& parameter,
		winrt::hstring const&)
	{
		if (!value) return box_value(t(L"Common.Unknown"));

		hstring format = parameter ? unbox_value_or<hstring>(parameter, hstring{}) : hstring{};
		auto formatFileTime = [](uint64_t value) -> winrt::Windows::Foundation::IInspectable {
			ULARGE_INTEGER raw{};
			raw.QuadPart = value;
			if (raw.QuadPart == 0) return box_value(t(L"Common.Unknown"));

			FILETIME fileTime{ raw.LowPart, raw.HighPart };
			SYSTEMTIME systemTime{};
			if (!FileTimeToSystemTime(&fileTime, &systemTime)) return box_value(t(L"Common.Unknown"));

			wchar_t text[32]{};
			swprintf_s(text, L"%04u/%02u/%02u %02u:%02u:%02u",
				systemTime.wYear, systemTime.wMonth, systemTime.wDay,
				systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
			return box_value(hstring(text));
		};
		if (format == L"FileSize") {
			auto file = value.try_as<winrt::StarlightGUI::FileInfo>();
			if (!file || file.Directory()) return box_value(hstring{});
			return box_value(hstring(FormatMemorySize((double)file.Size())));
		}
		if (format == L"FileModifyTime") {
			auto file = value.try_as<winrt::StarlightGUI::FileInfo>();
			if (!file || file.Flag() == 666) return box_value(hstring{});
			return formatFileTime(file.ModifyTime());
		}
		if (format.size() == 15 && std::wstring_view(format.c_str(), 14) == L"CallbackColumn") {
			auto entry = value.try_as<winrt::StarlightGUI::GeneralEntry>();
			if (!entry) return box_value(t(L"Common.Unknown"));

			auto column = format.c_str()[14] - L'0';
			auto type = (CallbackType)entry.ULong1();
			auto hex64 = [](uint64_t number) {
				return hstring(ULongToHexString(number));
			};
			auto hexCompact = [](uint64_t number) {
				return hstring(ULongToHexString(number, 0, true, true));
			};

			if (column == 1) return box_value(hex64(entry.ULongLong1()));
			if (type == CallbackType::CreateProcess || type == CallbackType::CreateThread ||
				type == CallbackType::LoadImage || type == CallbackType::LogonSessionTerminated ||
				type == CallbackType::DbgPrint) {
				if (column == 2) return box_value(to_hstring(entry.ULong2()));
				if (column == 3) return box_value(hexCompact(entry.ULong3()));
			}
			else if (type == CallbackType::Object && column == 4) {
				switch ((ObCallbackType)entry.ULong3()) {
				case ObCallbackType::Process: return box_value(hstring(L"Process"));
				case ObCallbackType::Thread: return box_value(hstring(L"Thread"));
				case ObCallbackType::Desktop: return box_value(hstring(L"Desktop"));
				default: return box_value(t(L"Common.Unknown"));
				}
			}
			else if (type == CallbackType::BugCheckReason) {
				if (column == 2) return box_value(hex64(entry.ULongLong3()));
				if (column == 3) return box_value(hexCompact(entry.ULongLong4()));
				if (column == 4) return box_value(hexCompact(entry.ULongLong2()));
			}
			else if (type == CallbackType::BugCheck && column == 4) {
				return box_value(hexCompact(entry.ULongLong4()));
			}

			switch (column) {
			case 2: return box_value(hex64(entry.ULongLong2()));
			case 3: return box_value(hex64(entry.ULongLong3()));
			case 4: return box_value(hex64(entry.ULongLong4()));
			default: return box_value(t(L"Common.Unknown"));
			}
		}

		auto property = value.try_as<winrt::Windows::Foundation::IPropertyValue>();
		if (!property) return value;

		auto type = property.Type();
		auto getUnsigned = [&]() -> uint64_t {
			switch (type) {
			case winrt::Windows::Foundation::PropertyType::UInt8: return property.GetUInt8();
			case winrt::Windows::Foundation::PropertyType::UInt16: return property.GetUInt16();
			case winrt::Windows::Foundation::PropertyType::UInt32: return property.GetUInt32();
			case winrt::Windows::Foundation::PropertyType::UInt64: return property.GetUInt64();
			case winrt::Windows::Foundation::PropertyType::Int16: return (uint64_t)property.GetInt16();
			case winrt::Windows::Foundation::PropertyType::Int32: return (uint32_t)property.GetInt32();
			case winrt::Windows::Foundation::PropertyType::Int64: return (uint64_t)property.GetInt64();
			default: return 0;
			}
		};
		auto getSigned = [&]() -> int64_t {
			switch (type) {
			case winrt::Windows::Foundation::PropertyType::UInt8: return property.GetUInt8();
			case winrt::Windows::Foundation::PropertyType::UInt16: return property.GetUInt16();
			case winrt::Windows::Foundation::PropertyType::UInt32: return property.GetUInt32();
			case winrt::Windows::Foundation::PropertyType::UInt64: return (int64_t)property.GetUInt64();
			case winrt::Windows::Foundation::PropertyType::Int16: return property.GetInt16();
			case winrt::Windows::Foundation::PropertyType::Int32: return property.GetInt32();
			case winrt::Windows::Foundation::PropertyType::Int64: return property.GetInt64();
			default: return 0;
			}
		};

		if (format == L"Hex64" || format == L"Hex64OrNone") {
			auto number = getUnsigned();
			if (format == L"Hex64OrNone" && number == 0) return box_value(t(L"Common.None"));
			return box_value(hstring(ULongToHexString(number)));
		}
		if (format == L"Hex32") {
			return box_value(hstring(ULongToHexString(getUnsigned(), 8, true, true)));
		}
		if (format == L"HexHandle") {
			return box_value(hstring(ULongToHexString((uint32_t)getUnsigned(), 8, true, true)));
		}
		if (format == L"HexCompact") {
			return box_value(hstring(ULongToHexString(getUnsigned(), 0, true, true)));
		}
		if (format == L"MemorySize" || format == L"MemorySizeOrUnknown") {
			auto number = getUnsigned();
			if (format == L"MemorySizeOrUnknown" && number == 0) return box_value(t(L"Common.Unknown"));
			return box_value(hstring(FormatMemorySize((double)number)));
		}
		if (format == L"FileTime") {
			return formatFileTime(getUnsigned());
		}
		if (format == L"CpuUsage") {
			double usage = type == winrt::Windows::Foundation::PropertyType::Double
				? property.GetDouble()
				: (double)getSigned();
			if (usage < 0) return box_value(t(L"Common.Unknown"));

			wchar_t text[32]{};
			swprintf_s(text, L"%.1f%%", usage);
			return box_value(hstring(text));
		}
		if (format == L"ThreadState") {
			static constexpr const wchar_t* keys[] = {
				L"Msg.Thread.Initialized", L"Msg.Thread.Ready", L"Msg.Thread.Running",
				L"Msg.Thread.Standby", L"Msg.Thread.Terminated", L"Msg.Thread.Waiting",
				L"Msg.Thread.Transition", L"Msg.Thread.DeferredReady", L"Msg.Thread.GateWait"
			};
			auto state = getUnsigned();
			return box_value(state < RTL_NUMBER_OF(keys) ? t(keys[state]) : t(L"Msg.Thread.Unknown"));
		}
		if (format == L"PreviousMode") {
			return box_value(hstring(getUnsigned() == 0 ? L"KernelMode" : L"UserMode"));
		}

		switch (type) {
		case winrt::Windows::Foundation::PropertyType::Double:
			return box_value(to_hstring(property.GetDouble()));
		case winrt::Windows::Foundation::PropertyType::Single:
			return box_value(to_hstring(property.GetSingle()));
		default:
			return box_value(to_hstring(getSigned()));
		}
	}

	winrt::Windows::Foundation::IInspectable DisplayValueConverter::ConvertBack(
		winrt::Windows::Foundation::IInspectable const&,
		winrt::Windows::UI::Xaml::Interop::TypeName const&,
		winrt::Windows::Foundation::IInspectable const&,
		winrt::hstring const&)
	{
		return winrt::Microsoft::UI::Xaml::DependencyProperty::UnsetValue();
	}
}

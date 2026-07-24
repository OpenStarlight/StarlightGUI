#pragma once

#include "GeneralEntry.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct GeneralEntry : GeneralEntryT<GeneralEntry>
	{
		GeneralEntry() = default;

		[[nodiscard]] hstring String1() const noexcept { return m_string1; }
		void String1(hstring const& value) { m_string1 = value; }

		[[nodiscard]] hstring String2() const noexcept { return m_string2; }
		void String2(hstring const& value) { m_string2 = value; }

		[[nodiscard]] hstring String3() const noexcept { return m_string3; }
		void String3(hstring const& value) { m_string3 = value; }

		[[nodiscard]] hstring String4() const noexcept { return m_string4; }
		void String4(hstring const& value) { m_string4 = value; }

		[[nodiscard]] hstring String5() const noexcept { return m_string5; }
		void String5(hstring const& value) { m_string5 = value; }

		[[nodiscard]] bool Bool1() const noexcept { return m_bool1; }
		void Bool1(bool value) { m_bool1 = value; }

		[[nodiscard]] bool Bool2() const noexcept { return m_bool2; }
		void Bool2(bool value) { m_bool2 = value; }

		[[nodiscard]] bool Bool3() const noexcept { return m_bool3; }
		void Bool3(bool value) { m_bool3 = value; }

		[[nodiscard]] bool Bool4() const noexcept { return m_bool4; }
		void Bool4(bool value) { m_bool4 = value; }

		[[nodiscard]] bool Bool5() const noexcept { return m_bool5; }
		void Bool5(bool value) { m_bool5 = value; }

		[[nodiscard]] LONG Long1() const noexcept { return m_long1; }
		void Long1(LONG value) { m_long1 = value; }

		[[nodiscard]] LONG Long2() const noexcept { return m_long2; }
		void Long2(LONG value) { m_long2 = value; }

		[[nodiscard]] LONG Long3() const noexcept { return m_long3; }
		void Long3(LONG value) { m_long3 = value; }

		[[nodiscard]] LONG Long4() const noexcept { return m_long4; }
		void Long4(LONG value) { m_long4 = value; }

		[[nodiscard]] LONG Long5() const noexcept { return m_long5; }
		void Long5(LONG value) { m_long5 = value; }

		[[nodiscard]] ULONG ULong1() const noexcept { return m_ulong1; }
		void ULong1(ULONG value) { m_ulong1 = value; }

		[[nodiscard]] ULONG ULong2() const noexcept { return m_ulong2; }
		void ULong2(ULONG value) { m_ulong2 = value; }

		[[nodiscard]] ULONG ULong3() const noexcept { return m_ulong3; }
		void ULong3(ULONG value) { m_ulong3 = value; }

		[[nodiscard]] ULONG ULong4() const noexcept { return m_ulong4; }
		void ULong4(ULONG value) { m_ulong4 = value; }

		[[nodiscard]] ULONG ULong5() const noexcept { return m_ulong5; }
		void ULong5(ULONG value) { m_ulong5 = value; }

		[[nodiscard]] LONG64 LongLong1() const noexcept { return m_longlong1; }
		void LongLong1(LONG64 value) { m_longlong1 = value; }

		[[nodiscard]] LONG64 LongLong2() const noexcept { return m_longlong2; }
		void LongLong2(LONG64 value) { m_longlong2 = value; }

		[[nodiscard]] LONG64 LongLong3() const noexcept { return m_longlong3; }
		void LongLong3(LONG64 value) { m_longlong3 = value; }

		[[nodiscard]] LONG64 LongLong4() const noexcept { return m_longlong4; }
		void LongLong4(LONG64 value) { m_longlong4 = value; }

		[[nodiscard]] LONG64 LongLong5() const noexcept { return m_longlong5; }
		void LongLong5(LONG64 value) { m_longlong5 = value; }

		[[nodiscard]] ULONG64 ULongLong1() const noexcept { return m_ulonglong1; }
		void ULongLong1(ULONG64 value) { m_ulonglong1 = value; }

		[[nodiscard]] ULONG64 ULongLong2() const noexcept { return m_ulonglong2; }
		void ULongLong2(ULONG64 value) { m_ulonglong2 = value; }

		[[nodiscard]] ULONG64 ULongLong3() const noexcept { return m_ulonglong3; }
		void ULongLong3(ULONG64 value) { m_ulonglong3 = value; }

		[[nodiscard]] ULONG64 ULongLong4() const noexcept { return m_ulonglong4; }
		void ULongLong4(ULONG64 value) { m_ulonglong4 = value; }

		[[nodiscard]] ULONG64 ULongLong5() const noexcept { return m_ulonglong5; }
		void ULongLong5(ULONG64 value) noexcept { m_ulonglong5 = value; }

	private:
		hstring m_string1{ L"" }, m_string2{ L"" }, m_string3{ L"" }, m_string4{ L"" }, m_string5{ L"" };
		bool m_bool1{ false }, m_bool2{ false }, m_bool3{ false }, m_bool4{ false }, m_bool5{ false };
		LONG m_long1{ 0 }, m_long2{ 0 }, m_long3{ 0 }, m_long4{ 0 }, m_long5{ 0 };
		ULONG m_ulong1{ 0 }, m_ulong2{ 0 }, m_ulong3{ 0 }, m_ulong4{ 0 }, m_ulong5{ 0 };
		LONG64 m_longlong1{ 0 }, m_longlong2{ 0 }, m_longlong3{ 0 }, m_longlong4{ 0 }, m_longlong5{ 0 };
		ULONG64 m_ulonglong1{ 0 }, m_ulonglong2{ 0 }, m_ulonglong3{ 0 }, m_ulonglong4{ 0 }, m_ulonglong5{ 0 };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct GeneralEntry : GeneralEntryT<GeneralEntry, implementation::GeneralEntry>
	{
	};
}

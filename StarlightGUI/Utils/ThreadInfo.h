#pragma once

#include "ThreadInfo.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct ThreadInfo : ThreadInfoT<ThreadInfo>
	{
		ThreadInfo() = default;

		int32_t Id() { return m_id; }
		void Id(int32_t value) { m_id = value; }

		hstring EThread() { return m_ethread; }
		void EThread(hstring const& value) { m_ethread = value; }

		hstring Address() { return m_address; }
		void Address(hstring const& value) { m_address = value; }

		hstring Win32Address() { return m_win32Address; }
		void Win32Address(hstring const& value) { m_win32Address = value; }

		hstring Status() { return m_status; }
		void Status(hstring const& value) { m_status = value; }

		int32_t Priority() { return m_priority; }
		void Priority(int32_t value) { m_priority = value; }

		hstring PreviousMode() { return m_previousMode; }
		void PreviousMode(hstring const& value) { m_previousMode = value; }

	private:
		int32_t m_id{ 0 };
		hstring m_ethread{ L"" };
		hstring m_address{ L"" };
		hstring m_win32Address{ L"" };
		hstring m_status{ L"" };
		int32_t m_priority{ 0 };
		hstring m_previousMode{ L"" };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct ThreadInfo : ThreadInfoT<ThreadInfo, implementation::ThreadInfo>
	{
	};
}
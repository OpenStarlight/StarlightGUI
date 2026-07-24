#pragma once

#include "ThreadInfo.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct ThreadInfo : ThreadInfoT<ThreadInfo>
	{
		ThreadInfo() = default;

		int32_t Id() { return m_id; }
		void Id(int32_t value) { m_id = value; }

		ULONG64 EThread() { return m_ethread; }
		void EThread(ULONG64 value) { m_ethread = value; }

		ULONG64 Address() { return m_address; }
		void Address(ULONG64 value) { m_address = value; }

		ULONG64 Win32Address() { return m_win32Address; }
		void Win32Address(ULONG64 value) { m_win32Address = value; }

		ULONG Status() { return m_status; }
		void Status(ULONG value) { m_status = value; }

		int32_t Priority() { return m_priority; }
		void Priority(int32_t value) { m_priority = value; }

		ULONG PreviousMode() { return m_previousMode; }
		void PreviousMode(ULONG value) { m_previousMode = value; }

	private:
		int32_t m_id{ 0 };
		ULONG64 m_ethread{ 0 };
		ULONG64 m_address{ 0 };
		ULONG64 m_win32Address{ 0 };
		ULONG m_status{ 0 };
		int32_t m_priority{ 0 };
		ULONG m_previousMode{ 0 };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct ThreadInfo : ThreadInfoT<ThreadInfo, implementation::ThreadInfo>
	{
	};
}

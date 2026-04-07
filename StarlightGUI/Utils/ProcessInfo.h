#pragma once

#include "ProcessInfo.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct ProcessInfo : ProcessInfoT<ProcessInfo>
	{
		ProcessInfo() = default;

		int32_t Id() { return m_id; }
		void Id(int32_t value) { m_id = value; }

		hstring Name() { return m_name; }
		void Name(hstring const& value) { m_name = value; }

		hstring Description() { return m_description; }
		void Description(hstring const& value) { m_description = value; }

		hstring MemoryUsage() { return m_memoryUsage; }
		void MemoryUsage(hstring const& value) { m_memoryUsage = value; }

		uint64_t MemoryUsageByte() { return m_memoryUsageByte; }
		void MemoryUsageByte(uint64_t value) { m_memoryUsageByte = value; }

		hstring CpuUsage() { return m_cpuUsage; }
		void CpuUsage(hstring const& value) { m_cpuUsage = value; }

		hstring ExecutablePath() { return m_executablePath; }
		void ExecutablePath(hstring const& value) { m_executablePath = value; }

		hstring EProcess() { return m_eprocess; }
		void EProcess(hstring const& value) { m_eprocess = value; }

		ULONG64 EProcessULong() { return m_eprocessULong; }
		void EProcessULong(ULONG64 value) { m_eprocessULong = value; }

		winrt::Microsoft::UI::Xaml::Media::ImageSource Icon() { return m_icon; }
		void Icon(winrt::Microsoft::UI::Xaml::Media::ImageSource const& value) { m_icon = value; }

	private:
		int32_t m_id{ 0 };
		hstring m_name{ L"" };
		hstring m_description{ L"" };
		hstring m_memoryUsage{ L"" };
		uint64_t m_memoryUsageByte{ 0 };
		hstring m_cpuUsage{ L"" };
		hstring m_executablePath{ L"" };
		hstring m_eprocess{ L"" };
		ULONG64 m_eprocessULong{ 0 };
		winrt::Microsoft::UI::Xaml::Media::ImageSource m_icon{ nullptr };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct ProcessInfo : ProcessInfoT<ProcessInfo, implementation::ProcessInfo>
	{
	};
}
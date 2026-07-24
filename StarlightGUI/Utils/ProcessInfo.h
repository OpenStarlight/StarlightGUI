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

		uint64_t MemoryUsage() { return m_memoryUsage; }
		void MemoryUsage(uint64_t value) { m_memoryUsage = value; }

		double CpuUsage() { return m_cpuUsage; }
		void CpuUsage(double value) { m_cpuUsage = value; }

		hstring ExecutablePath() { return m_executablePath; }
		void ExecutablePath(hstring const& value) { m_executablePath = value; }

		ULONG64 EProcess() { return m_eprocess; }
		void EProcess(ULONG64 value) { m_eprocess = value; }

		winrt::Microsoft::UI::Xaml::Media::ImageSource Icon() { return m_icon; }
		void Icon(winrt::Microsoft::UI::Xaml::Media::ImageSource const& value) { m_icon = value; }

	private:
		int32_t m_id{ 0 };
		hstring m_name{ L"" };
		hstring m_description{ L"" };
		uint64_t m_memoryUsage{ 0 };
		double m_cpuUsage{ -1.0 };
		hstring m_executablePath{ L"" };
		ULONG64 m_eprocess{ 0 };
		winrt::Microsoft::UI::Xaml::Media::ImageSource m_icon{ nullptr };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct ProcessInfo : ProcessInfoT<ProcessInfo, implementation::ProcessInfo>
	{
	};
}

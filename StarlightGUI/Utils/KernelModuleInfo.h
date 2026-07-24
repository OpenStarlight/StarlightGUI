#pragma once

#include "KernelModuleInfo.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct KernelModuleInfo : KernelModuleInfoT<KernelModuleInfo>
	{
		KernelModuleInfo() = default;

		hstring Name() { return m_name; }
		void Name(hstring const& value) { m_name = value; }

		hstring Path() { return m_path; }
		void Path(hstring const& value) { m_path = value; }

		ULONG64 ImageBase() { return m_imageBase; }
		void ImageBase(ULONG64 value) { m_imageBase = value; }

		ULONG64 Size() { return m_size; }
		void Size(ULONG64 value) { m_size = value; }

		ULONG64 DriverObject() { return m_driverObject; }
		void DriverObject(ULONG64 value) { m_driverObject = value; }

	private:
		hstring m_name{ L"" };
		hstring m_path{ L"" };
		ULONG64 m_imageBase{ 0 };
		ULONG64 m_size{ 0 };
		ULONG64 m_driverObject{ 0 };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct KernelModuleInfo : KernelModuleInfoT<KernelModuleInfo, implementation::KernelModuleInfo>
	{
	};
}

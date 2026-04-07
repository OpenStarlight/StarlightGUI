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

		hstring ImageBase() { return m_imageBase; }
		void ImageBase(hstring const& value) { m_imageBase = value; }

		ULONG64 ImageBaseULong() { return m_imageBaseULong; }
		void ImageBaseULong(ULONG64 const& value) { m_imageBaseULong = value; }

		hstring Size() { return m_size; }
		void Size(hstring const& value) { m_size = value; }

		ULONG64 SizeULong() { return m_sizeULong; }
		void SizeULong(ULONG64 value) { m_sizeULong = value; }

		hstring DriverObject() { return m_driverObject; }
		void DriverObject(hstring const& value) { m_driverObject = value; }

		ULONG64 DriverObjectULong() { return m_driverObjectULong; }
		void DriverObjectULong(ULONG64 value) { m_driverObjectULong = value; }

		ULONG64 Index() { return m_index; }
		void Index(ULONG64 value) { m_index = value; }

	private:
		hstring m_name{ L"" };
		hstring m_path{ L"" };
		hstring m_imageBase{ L"" };
		ULONG64 m_imageBaseULong{ 0 };
		hstring m_size{ L"" };
		ULONG64 m_sizeULong{ 0 };
		hstring m_driverObject{ L"" };
		ULONG64 m_driverObjectULong{ 0 };
		ULONG64 m_index{ 0 };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct KernelModuleInfo : KernelModuleInfoT<KernelModuleInfo, implementation::KernelModuleInfo>
	{
	};
}
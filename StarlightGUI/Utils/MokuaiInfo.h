#pragma once

#include "MokuaiInfo.g.h"
/*
* 补药问我为什么是Mokuai不是Module，ModuleInfo会报错，我不知道为什么
* @Author Stars
*/
namespace winrt::StarlightGUI::implementation
{
	struct MokuaiInfo : MokuaiInfoT<MokuaiInfo>
	{
		MokuaiInfo() = default;

		hstring Name() { return m_name; }
		void Name(hstring const& value) { m_name = value; }

		ULONG64 Address() { return m_address; }
		void Address(ULONG64 value) { m_address = value; }

		ULONG64 Size() { return m_size; }
		void Size(ULONG64 value) { m_size = value; }

		hstring Path() { return m_path; }
		void Path(hstring const& value) { m_path = value; }

	private:
		hstring m_name{ L"" };
		ULONG64 m_address{ 0 };
		ULONG64 m_size{ 0 };
		hstring m_path{ L"" };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct MokuaiInfo : MokuaiInfoT<MokuaiInfo, implementation::MokuaiInfo>
	{
	};
}

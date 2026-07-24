#pragma once

#include "KCTInfo.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct KCTInfo : KCTInfoT<KCTInfo>
	{
		KCTInfo() = default;

		hstring Name() { return m_name; }
		void Name(hstring const& value) { m_name = value; }

		ULONG64 Address() { return m_address; }
		void Address(ULONG64 value) { m_address = value; }

	private:
		hstring m_name{ L"" };
		ULONG64 m_address{ 0 };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct KCTInfo : KCTInfoT<KCTInfo, implementation::KCTInfo>
	{
	};
}

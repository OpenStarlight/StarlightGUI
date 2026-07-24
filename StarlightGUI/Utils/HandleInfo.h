#pragma once

#include "HandleInfo.g.h"

namespace winrt::StarlightGUI::implementation
{
	struct HandleInfo : HandleInfoT<HandleInfo>
	{
		HandleInfo() = default;

		hstring Type() { return m_type; }
		void Type(hstring const& value) { m_type = value; }

		ULONG64 Object() { return m_object; }
		void Object(ULONG64 value) { m_object = value; }

		ULONG64 Handle() { return m_handle; }
		void Handle(ULONG64 value) { m_handle = value; }

		ULONG Access() { return m_access; }
		void Access(ULONG value) { m_access = value; }

		ULONG Attributes() { return m_attributes; }
		void Attributes(ULONG value) { m_attributes = value; }

	private:
		hstring m_type{ L"" };
		ULONG64 m_object{ 0 };
		ULONG64 m_handle{ 0 };
		ULONG m_access{ 0 };
		ULONG m_attributes{ 0 };
	};
}

namespace winrt::StarlightGUI::factory_implementation
{
	struct HandleInfo : HandleInfoT<HandleInfo, implementation::HandleInfo>
	{
	};
}

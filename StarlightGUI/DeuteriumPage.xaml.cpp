#include "pch.h"
#include "DeuteriumPage.xaml.h"
#if __has_include("DeuteriumPage.g.cpp")
#include "DeuteriumPage.g.cpp"
#endif

#include <limits>

#undef max
#undef min

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    static bool loaded = false;

    DeuteriumPage::DeuteriumPage() {
		InitializeComponent();

        StackListView().ItemsSource(m_stackList);
        if (!list_animation) StackListView().ItemContainerTransitions().Clear();

        Loaded([this](auto&&, auto&&){
            if (!loaded) {
                loaded = true;
                auto dialog = CreateContentDialog(L"警告",
                    L"您正在访问的是一个极度危险的功能: Deuterium Proxy\n\n"
                    "该功能仅供专业用户使用！如果您不了解，请立刻离开！\n"
                    "任意一个操作都有极大的可能导致你的系统立刻崩溃！！\n"
                    "在本页面执行的任何操作，都请经过充分考虑，否则后果自负！！！",
                    L"关闭", XamlRoot());
                dialog.TitleTemplate(GetContentDialogErrorTemplate());
                dialog.ShowAsync();
            }
			});

        LOG_INFO(L"DeuteriumPage", L"DeuteriumPage initialized.");
    }

    IAsyncAction DeuteriumPage::ExecuteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        DEUTERIUM_PROXY_INVOKE function{};
        if (!BuildDeuteriumInvokeRequest(function)) {
            CreateInfoBarAndDisplay(L"失败", L"调用失败: 构建请求时出错", InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
        }

        if (DeuteriumInvoke(function)) {
            CreateInfoBarAndDisplay(L"成功", L"调用成功！", InfoBarSeverity::Success, g_mainWindowInstance);
        }
        else {
            CreateInfoBarAndDisplay(L"失败", L"调用失败: 执行请求时出错", InfoBarSeverity::Error, g_mainWindowInstance);
		}

        co_return;
    }

    IAsyncAction DeuteriumPage::AllocButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (AllocSizeSlider().Value() < 4 || AllocSizeSlider().Value() > 64) {
            CreateInfoBarAndDisplay(L"失败", L"分配失败: 大小不合法", InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
        }

        DEUTERIUM_PROXY_ALLOCATE function{};
        function.Size = (ULONG)AllocSizeSlider().Value();

        BOOL status = KernelInstance::DeuteriumAlloc(function, AllocMapBox().IsChecked().GetBoolean());

        if (status) {
            if (function.Address) {
                auto entry = make<winrt::StarlightGUI::implementation::GeneralEntry>();
                entry.String1(ULongToHexString(function.Address));
                entry.ULong1(function.Size);
                entry.Bool1(AllocMapBox().IsChecked().GetBoolean());
                m_stackList.Append(entry);
                CreateInfoBarAndDisplay(L"成功", L"分配成功！请在列表里查看地址！", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else {
                CreateInfoBarAndDisplay(L"失败", L"分配失败: 执行请求成功，但返回地址为空！", InfoBarSeverity::Success, g_mainWindowInstance);
            }
        }
        else {
            CreateInfoBarAndDisplay(L"失败", L"分配失败: 执行请求时出错, Error: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
        }

        co_return;
    }

    IAsyncAction DeuteriumPage::FreeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        DEUTERIUM_PROXY_FREE function{};
        
        if (!HexStringToULong(FreeAddressBox().Text().c_str(), function.Address)) {
            CreateInfoBarAndDisplay(L"失败", L"释放失败: 地址不合法", InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
        }

        uint32_t index = 114514;
        for (const auto& stack : m_stackList) {
            ULONG64 address = 0;
            if (HexStringToULong(stack.String1().c_str(), address)) {
                if (address == function.Address) {
                    m_stackList.IndexOf(stack, index);
                    break;
                }
            }
        }

        if (index == 114514) {
            CreateInfoBarAndDisplay(L"失败", L"释放失败: 地址不存在于列表中", InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
        }

        BOOL status = KernelInstance::DeuteriumFree(function, FreeMapBox().IsChecked().GetBoolean());

        if (status) {
            m_stackList.RemoveAt(index);
            CreateInfoBarAndDisplay(L"成功", L"释放成功！", InfoBarSeverity::Success, g_mainWindowInstance);
        }
        else {
            CreateInfoBarAndDisplay(L"失败", L"释放失败: 执行请求时出错", InfoBarSeverity::Error, g_mainWindowInstance);
        }

        co_return;
    }

    void DeuteriumPage::ParamCountSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        int value = e.NewValue();

        Param1Expander().IsEnabled(value > 0);
        Param2Expander().IsEnabled(value > 1);
        Param3Expander().IsEnabled(value > 2);
        Param4Expander().IsEnabled(value > 3);
        Param5Expander().IsEnabled(value > 4);
        Param6Expander().IsEnabled(value > 5);
        Param7Expander().IsEnabled(value > 6);
        Param8Expander().IsEnabled(value > 7);
        Param9Expander().IsEnabled(value > 8);
        Param10Expander().IsEnabled(value > 9);
    }

    bool DeuteriumPage::CheckCode(int code, int index) {
        if (code == 0) return true;
        else if (code == 1) {
            CreateInfoBarAndDisplay(L"错误", L"参数 " + to_hstring(index) + L" 错误: 输入的值与类型不匹配！", InfoBarSeverity::Error, g_mainWindowInstance);
            return false;
        }
        else if (code == 2) {
            CreateInfoBarAndDisplay(L"错误", L"参数 " + to_hstring(index) + L" 错误: 输入为空！", InfoBarSeverity::Error, g_mainWindowInstance);
            return false;
        }
        else if (code == 3) {
            CreateInfoBarAndDisplay(L"错误", L"参数 " + to_hstring(index) + L" 错误: 未知的参数类型！", InfoBarSeverity::Error, g_mainWindowInstance);
            return false;
        }
        else if (code == 4) {
            CreateInfoBarAndDisplay(L"错误", L"参数 " + to_hstring(index) + L" 错误: 输入的值对于参数类型会产生溢出！", InfoBarSeverity::Error, g_mainWindowInstance);
            return false;
        }

        CreateInfoBarAndDisplay(L"错误", L"参数 " + to_hstring(index) + L" 错误: 未知错误！", InfoBarSeverity::Error, g_mainWindowInstance);
        return false;
    }

    bool DeuteriumPage::DeuteriumInvoke(DEUTERIUM_PROXY_INVOKE& function) {
		PVOID retAddr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PVOID));
        if (retAddr) {
            function.Function.ReturnValueAddress = (ULONG64)retAddr;
        }

		BOOL result = KernelInstance::DeuteriumInvoke(function);

        HeapFree(GetProcessHeap(), 0, retAddr);
		return result;
    }

    bool DeuteriumPage::BuildDeuteriumInvokeRequest(DEUTERIUM_PROXY_INVOKE& function) {
        // 检查函数内容
        DEUTERIUM_FUNCTION func = { TYPE_VOID, TYPE_NONE, 0, 0 };
        hstring addrText = FunctionAddressBox().Text();
        ULONG64 addr = 0;
        if (!HexStringToULong(addrText.c_str(), addr)) {
            CreateInfoBarAndDisplay(L"错误", L"未知的函数地址！", InfoBarSeverity::Error, g_mainWindowInstance);
            return false;
        }

        ULONG retType = FunctionReturnBox().SelectedIndex() + 1;
        if (retType < 1 || retType > 13) {
            CreateInfoBarAndDisplay(L"错误", L"未知的返回类型！", InfoBarSeverity::Error, g_mainWindowInstance);
            return false;
        }

		func.FunctionAddress = addr;
		func.ReturnVarType = (DEUTERIUM_PROXY_VAR_TYPE)retType;

        // 检查参数内容
        ULONG sliderValue = ParamCountSlider().Value();
		DEUTERIUM_MEMBER param1{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param2{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param3{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param4{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param5{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param6{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param7{ TYPE_EMPTY , TYPE_ALL, TYPE_NONE, 0 }, param8{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param9{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 }, param10{ TYPE_EMPTY, TYPE_ALL, TYPE_NONE, 0 };
        if (sliderValue > 0) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param1TypeBox().SelectedIndex() + 1), Param1ValueBox().Text()), 1)) return false;
            if (Param1InOutTypeBox().SelectedIndex() < 0 || Param1InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 1 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param1ReferenceTypeBox().SelectedIndex() < 0 || Param1ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 1 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            StringToNumber(Param1ValueBox().Text().c_str(), param1.MemberAddress);
            param1.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param1TypeBox().SelectedIndex() + 1);
            param1.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param1InOutTypeBox().SelectedIndex();
            param1.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param1ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 1) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param2TypeBox().SelectedIndex() + 1), Param2ValueBox().Text()), 1)) return false;
            if (Param2InOutTypeBox().SelectedIndex() < 0 || Param2InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 2 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param2ReferenceTypeBox().SelectedIndex() < 0 || Param2ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 2 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            StringToNumber(Param2ValueBox().Text().c_str(), param2.MemberAddress);
            param2.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param2TypeBox().SelectedIndex() + 1);
            param2.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param2InOutTypeBox().SelectedIndex();
            param2.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param2ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 2) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param3TypeBox().SelectedIndex() + 1), Param3ValueBox().Text()), 1)) return false;
            if (Param3InOutTypeBox().SelectedIndex() < 0 || Param3InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 3 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param3ReferenceTypeBox().SelectedIndex() < 0 || Param3ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 3 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param3ValueBox().Text().c_str(), param3.MemberAddress);
            param3.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param3TypeBox().SelectedIndex() + 1);
            param3.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param3InOutTypeBox().SelectedIndex();
			param3.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param3ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 3) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param4TypeBox().SelectedIndex() + 1), Param4ValueBox().Text()), 1)) return false;
            if (Param4InOutTypeBox().SelectedIndex() < 0 || Param4InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 4 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param4ReferenceTypeBox().SelectedIndex() < 0 || Param4ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 4 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param4ValueBox().Text().c_str(), param4.MemberAddress);
            param4.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param4TypeBox().SelectedIndex() + 1);
			param4.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param4InOutTypeBox().SelectedIndex();
			param4.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param4ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 4) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param5TypeBox().SelectedIndex() + 1), Param5ValueBox().Text()), 1)) return false;
            if (Param5InOutTypeBox().SelectedIndex() < 0 || Param5InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 5 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param5ReferenceTypeBox().SelectedIndex() < 0 || Param5ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 5 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param5ValueBox().Text().c_str(), param5.MemberAddress);
			param5.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param5TypeBox().SelectedIndex() + 1);
			param5.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param5InOutTypeBox().SelectedIndex();
			param5.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param5ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 5) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param6TypeBox().SelectedIndex() + 1), Param6ValueBox().Text()), 1)) return false;
            if (Param6InOutTypeBox().SelectedIndex() < 0 || Param6InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 6 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param6ReferenceTypeBox().SelectedIndex() < 0 || Param6ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 6 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param6ValueBox().Text().c_str(), param6.MemberAddress);
			param6.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param6TypeBox().SelectedIndex() + 1);
			param6.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param6InOutTypeBox().SelectedIndex();
			param6.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param6ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 6) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param7TypeBox().SelectedIndex() + 1), Param7ValueBox().Text()), 1)) return false;
            if (Param7InOutTypeBox().SelectedIndex() < 0 || Param7InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 7 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param7ReferenceTypeBox().SelectedIndex() < 0 || Param7ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 7 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param7ValueBox().Text().c_str(), param7.MemberAddress);
			param7.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param7TypeBox().SelectedIndex() + 1);
			param7.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param7InOutTypeBox().SelectedIndex();
			param7.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param7ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 7) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param8TypeBox().SelectedIndex() + 1), Param8ValueBox().Text()), 1)) return false;
            if (Param8InOutTypeBox().SelectedIndex() < 0 || Param8InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 8 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param8ReferenceTypeBox().SelectedIndex() < 0 || Param8ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 8 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param8ValueBox().Text().c_str(), param8.MemberAddress);
			param8.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param8TypeBox().SelectedIndex() + 1);
			param8.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param8InOutTypeBox().SelectedIndex();
			param8.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param8ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 8) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param9TypeBox().SelectedIndex() + 1), Param9ValueBox().Text()), 1)) return false;
            if (Param9InOutTypeBox().SelectedIndex() < 0 || Param9InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 9 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param9ReferenceTypeBox().SelectedIndex() < 0 || Param9ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 9 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param9ValueBox().Text().c_str(), param9.MemberAddress);
			param9.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param9TypeBox().SelectedIndex() + 1);
			param9.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param9InOutTypeBox().SelectedIndex();
			param9.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param9ReferenceTypeBox().SelectedIndex();
        }
        if (sliderValue > 9) {
            if (!CheckCode(CheckParam((DEUTERIUM_PROXY_VAR_TYPE)(Param10TypeBox().SelectedIndex() + 1), Param10ValueBox().Text()), 1)) return false;
            if (Param10InOutTypeBox().SelectedIndex() < 0 || Param10InOutTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 10 错误: 未知的 IN/OUT 类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
            if (Param10ReferenceTypeBox().SelectedIndex() < 0 || Param10ReferenceTypeBox().SelectedIndex() > 2) {
                CreateInfoBarAndDisplay(L"错误", L"参数 10 错误: 未知的引用类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                return false;
            }
			StringToNumber(Param10ValueBox().Text().c_str(), param10.MemberAddress);
			param10.MemberVarType = (DEUTERIUM_PROXY_VAR_TYPE)(Param10TypeBox().SelectedIndex() + 1);
			param10.InOutType = (DEUTERIUM_PROXY_INOUT_TYPE)Param10InOutTypeBox().SelectedIndex();
			param10.ReferenceType = (DEUTERIUM_PROXY_REFERENCE_TYPE)Param10ReferenceTypeBox().SelectedIndex();
        }

        function.Function = func;
		function.ShouldAttach = FunctionAttachBox().IsChecked().GetBoolean();
		function.Param1 = param1;
		function.Param2 = param2;
		function.Param3 = param3;
		function.Param4 = param4;
		function.Param5 = param5;
        function.Param6 = param6;
        function.Param7 = param7;
        function.Param8 = param8;
        function.Param9 = param9;
        function.Param10 = param10;

        return true;
    }

    /*
 * 0 = 成功
 * 1 = 输入不匹配
 * 2 = 输入为空
 * 3 = 未知类型
 * 4 = 溢出
 */
    int DeuteriumPage::CheckParam(DEUTERIUM_PROXY_VAR_TYPE varType, hstring value) {
        if (varType == TYPE_NULLABLE) return 0;
        if (value.empty()) return 2;
        switch (varType) {
        case TYPE_LONG: {
            LONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<LONG>::max() || v < std::numeric_limits<LONG>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_LONG64: {
            LONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<LONG64>::max() || v < std::numeric_limits<LONG64>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_ULONG: {
            ULONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<ULONG>::max() || v < std::numeric_limits<ULONG>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_ULONG64: {
            ULONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<ULONG64>::max() || v < std::numeric_limits<ULONG64>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_CHAR: {
            LONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<CHAR>::max() || v < std::numeric_limits<CHAR>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_UCHAR: {
            ULONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<UCHAR>::max() || v < std::numeric_limits<UCHAR>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_WCHAR: {
            ULONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<WCHAR>::max() || v < std::numeric_limits<WCHAR>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_SHORT: {
            LONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<SHORT>::max() || v < std::numeric_limits<SHORT>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_USHORT: {
            LONG64 v = 0;
            if (StringToNumber(value.c_str(), v)) {
                if (v > std::numeric_limits<USHORT>::max() || v < std::numeric_limits<USHORT>::min()) return 4;
                return 0;
            }
            else return 1;
            break;
        }
        case TYPE_BOOLEAN: {
            if (value == L"true" || value == L"false" || value == L"TRUE" || value == L"FALSE") return 0;
            else return 1;
            break;
        }
        case TYPE_PVOID: {
            ULONG64 v = 0;
            if (HexStringToULong(value.c_str(), v)) {
                if ((v > 0x0ULL && v <= 0x00007FFFFFFFFFFFULL) || (v >= 0xFFFF800000000000ULL || v <= 0xFFFFFFFFFFFFFFFFULL)) return 0;
                else return 4;
            }
            else return 1;
            break;
        }
        default: {
            return 3;
            break;
        }
        }
    }

    void DeuteriumPage::StackListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue())
            return;

        // 将 Tag 设到容器上，便于 ListViewItemPresenter 通过 TemplatedParent 绑定
        if (auto itemContainer = args.ItemContainer())
            itemContainer.Tag(sender.Tag());
    }
}
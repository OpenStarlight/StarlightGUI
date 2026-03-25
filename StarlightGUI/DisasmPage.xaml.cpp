#include "pch.h"
#include "DisasmPage.xaml.h"
#if __has_include("DisasmPage.g.cpp")
#include "DisasmPage.g.cpp"
#endif

#include <iomanip>
#include <cwctype>
#include <MainWindow.xaml.h>
#include <capstone/capstone.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    bool confirmed = false;

    DisasmPage::DisasmPage()
    {
        InitializeComponent();

        DisasmTitleUid().Text(GetLocalizedString(L"Disasm_Title.Text"));
        DisasmModeReadUid().Content(tbox(L"Disasm_ModeRead.Content"));
        DisasmModeReadDisasmUid().Content(tbox(L"Disasm_ModeReadDisasm.Content"));
        DisasmModeWriteUid().Content(tbox(L"Disasm_ModeWrite.Content"));
        DisasmExecuteUid().Text(GetLocalizedString(L"Disasm_Execute.Text"));
        AddressBox().Header(tbox(L"Disasm_AddressBox.Header"));
        SizeBox().Header(tbox(L"Disasm_SizeBox.Header"));
        ValueBox().Header(tbox(L"Disasm_ValueBox.Header"));
        HexText().Text(GetLocalizedString(L"Disasm_None.Text"));
    }

    slg::coroutine DisasmPage::Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        int mode = ModeComboBox().SelectedIndex();

        if (mode == 0 || mode == 1) {
            ULONG64 address = 0, size = 0;
            if (!HexStringToULong(AddressBox().Text().c_str(), address) || !StringToNumber(SizeBox().Text().c_str(), size)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Disasm_InvalidInput").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                co_return;
            }

            co_await winrt::resume_background();

            std::vector<BYTE> buffer(size);

            BOOL result = KernelInstance::ReadMemory(buffer, (PVOID)address, (ULONG)size);

            co_await wil::resume_foreground(DispatcherQueue());

            if (result)
            {
                if (mode == 0) {
                    int counter = 0;
                    std::wstringstream byteStream, charStream;
                    for (BYTE section : buffer) {
                        counter++;
                        byteStream << std::setw(2) << std::setfill(L'0') << std::hex << std::uppercase << (int)section << L" ";
                        if (std::iswprint(section)) charStream << (wchar_t)section;
                        else charStream << L".";
                        if (counter >= disasm_count) {
                            byteStream << L"\n";
                            charStream << L"\n";
                            counter = 0;
                        }
                    }
                    HexText().MinWidth(100);
                    HexText().Text(byteStream.str());
                    CharText().MinWidth(100);
                    CharText().Text(charStream.str());
                }
                else {
                    csh handle;
                    cs_err a = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
                    if (a != CS_ERR_OK) {
                        slg::CreateInfoBarAndDisplay(
                            GetLocalizedString(L"Msg_Error").c_str(),
                            GetLocalizedString(L"Disasm_CapstoneInitFailed").c_str() + to_hstring((int)a),
                            InfoBarSeverity::Error,
                            g_mainWindowInstance
                        );
                        co_return;
                    }

                    cs_insn* insn = nullptr;
                    size_t count = cs_disasm(handle, buffer.data(), buffer.size(), address, 0, &insn);

                    if (count > 0) {
                        std::wstringstream byteStream, disasmStream;

                        for (size_t i = 0; i < count; ++i) {
                            byteStream << L"(" << ULongToHexString(insn[i].address).c_str() << L") ";
                            for (size_t j = 0; j < insn[i].size; ++j) {
                                byteStream << std::setw(2)
                                    << std::setfill(L'0')
                                    << std::hex
                                    << std::uppercase
                                    << static_cast<int>(insn[i].bytes[j])
                                    << L" ";
                            }
                            byteStream << L"\n";

                            std::wstring mnemonic(insn[i].mnemonic, insn[i].mnemonic + strlen(insn[i].mnemonic));
                            std::wstring opstr(insn[i].op_str, insn[i].op_str + strlen(insn[i].op_str));

                            disasmStream << mnemonic;
                            if (!opstr.empty()) {
                                disasmStream << L" " << opstr;
                            }
                            disasmStream << L"\n";
                        }

                        HexText().MinWidth(10);
                        HexText().Text(byteStream.str());

                        CharText().MinWidth(100);
                        CharText().Text(disasmStream.str());

                        cs_free(insn, count);
                    }
                    else {
                        slg::CreateInfoBarAndDisplay(
                            GetLocalizedString(L"Msg_Warning").c_str(),
                            GetLocalizedString(L"Disasm_NoInstructions").c_str(),
                            InfoBarSeverity::Warning,
                            g_mainWindowInstance
                        );
                    }

                    cs_close(&handle);
                }
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Disasm_QuerySuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else
            {
                HexText().Text(GetLocalizedString(L"Disasm_NA"));
                CharText().Text(GetLocalizedString(L"Disasm_NA"));
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Disasm_QueryFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
        else {
            if (!confirmed) {
                slg::CreateInfoBarAndDisplay(
                    GetLocalizedString(L"Msg_Warning").c_str(),
                    GetLocalizedString(L"Disasm_WriteWarning").c_str(),
                    InfoBarSeverity::Warning,
                    g_mainWindowInstance
                );
				confirmed = true;
                co_return;
			}
            ULONG64 address = 0, size = 0, data = 0;
            if (!HexStringToULong(AddressBox().Text().c_str(), address) || !StringToNumber(SizeBox().Text().c_str(), size) || !StringToNumber(ValueBox().Text().c_str(), data)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Disasm_InvalidInput").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                co_return;
            }

            co_await winrt::resume_background();

            BOOL result = KernelInstance::WriteMemory((PVOID)address, (PVOID)data, (ULONG)size);

            co_await wil::resume_foreground(DispatcherQueue());

            if (result)
            {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Disasm_WriteSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else
            {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Disasm_WriteFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
    }
}




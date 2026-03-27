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
        SetupLocalization();
    }

    slg::coroutine DisasmPage::Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        int mode = ModeComboBox().SelectedIndex();

        if (mode == 0 || mode == 1) {
            ULONG64 address = 0, size = 0;
            if (!HexStringToULong(AddressBox().Text().c_str(), address) || !StringToNumber(SizeBox().Text().c_str(), size)) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Disasm.Msg.InvalidInput"), InfoBarSeverity::Error, g_mainWindowInstance);
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
                            t(L"Common.Error"),
                            t(L"Disasm.Msg.CapstoneInitFailed") + to_hstring((int)a),
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
                            t(L"Common.Warning"),
                            t(L"Disasm.Msg.NoInstructions"),
                            InfoBarSeverity::Warning,
                            g_mainWindowInstance
                        );
                    }

                    cs_close(&handle);
                }
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else
            {
                HexText().Text(t(L"Common.None"));
                CharText().Text(t(L"Common.None"));
                slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
        else {
            if (!confirmed) {
                slg::CreateInfoBarAndDisplay(
                    t(L"Common.Warning").c_str(),
                    t(L"Disasm.Msg.WriteWarning").c_str(),
                    InfoBarSeverity::Warning,
                    g_mainWindowInstance
                );
				confirmed = true;
                co_return;
			}
            ULONG64 address = 0, size = 0, data = 0;
            if (!HexStringToULong(AddressBox().Text().c_str(), address) || !StringToNumber(SizeBox().Text().c_str(), size) || !StringToNumber(ValueBox().Text().c_str(), data)) {
                slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Disasm.Msg.InvalidInput"), InfoBarSeverity::Error, g_mainWindowInstance);
                co_return;
            }

            co_await winrt::resume_background();

            BOOL result = KernelInstance::WriteMemory((PVOID)address, (PVOID)data, (ULONG)size);

            co_await wil::resume_foreground(DispatcherQueue());

            if (result)
            {
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else
            {
                slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
    }

    void DisasmPage::SetupLocalization() {
        DisasmTitleUid().Text(t(L"Disasm.Title"));
        DisasmModeReadUid().Content(tbox(L"Disasm.Menu.ModeRead"));
        DisasmModeReadDisasmUid().Content(tbox(L"Disasm.Menu.ModeReadDisasm"));
        DisasmModeWriteUid().Content(tbox(L"Disasm.Menu.ModeWrite"));
        DisasmExecuteUid().Text(t(L"Disasm.Execute"));
        AddressBox().Header(tbox(L"Common.Address"));
        SizeBox().Header(tbox(L"Common.Size"));
        ValueBox().Header(tbox(L"Common.Value"));
        HexText().Text(t(L"Common.None"));
		CharText().Text(t(L"Common.None"));
    }
}




#include "pch.h"
#include "UtilityPage.xaml.h"
#if __has_include("UtilityPage.g.cpp")
#include "UtilityPage.g.cpp"
#endif

#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation{
	static hstring safeAcceptedTag = L"";

	UtilityPage::UtilityPage() {
		InitializeComponent();
		SetupLocalization();

		LOG_INFO(L"UtilityPage", L"UtilityPage initialized.");
	}

	slg::coroutine UtilityPage::Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		auto button = sender.as<Button>();
		std::wstring tag = button.Tag().as<winrt::hstring>().c_str();

		if (safeAcceptedTag != tag) {
			safeAcceptedTag = tag;
			slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"Utility.Msg.ConfirmAction").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
			co_return;
		}

		ULONG color = BSODColorComboBox().SelectedIndex() - 1;
		ULONG type = PGTypeComboBox().SelectedIndex();

		co_await winrt::resume_background();

		LOG_INFO(L"UtilityPage", L"Confirmed we will do: %s", tag.c_str());

		BOOL result = FALSE;

		if (tag == L"ENABLE_HVM") {
			result = KernelInstance::EnableHVM();
			hypervisor_mode = result;
		}
		else if (tag == L"ENABLE_CREATE_PROCESS") {
			result = KernelInstance::EnableCreateProcess();
		}
		else if (tag == L"DISABLE_CREATE_PROCESS") {
			result = KernelInstance::DisableCreateProcess();
		}
		else if (tag == L"ENABLE_CREATE_FILE") {
			result = KernelInstance::EnableCreateFile();
		}
		else if (tag == L"DISABLE_CREATE_FILE") {
			result = KernelInstance::DisableCreateFile();
		}
		else if (tag == L"ENABLE_LOAD_DRV") {
			result = KernelInstance::EnableLoadDriver();
		}
		else if (tag == L"DISABLE_LOAD_DRV") {
			result = KernelInstance::DisableLoadDriver();
		}
		else if (tag == L"ENABLE_UNLOAD_DRV") {
			result = KernelInstance::EnableUnloadDriver();
		}
		else if (tag == L"DISABLE_UNLOAD_DRV") {
			result = KernelInstance::DisableUnloadDriver();
		}
		else if (tag == L"ENABLE_MODIFY_REG") {
			result = KernelInstance::EnableModifyRegistry();
		}
		else if (tag == L"DISABLE_MODIFY_REG") {
			result = KernelInstance::DisableModifyRegistry();
		}
		else if (tag == L"ENABLE_MODIFY_BOOTSEC") {
			result = KernelInstance::ProtectDisk();
		}
		else if (tag == L"DISABLE_MODIFY_BOOTSEC") {
			result = KernelInstance::UnprotectDisk();
		}
		else if (tag == L"ENABLE_OBJ_REG_CB") {
			result = KernelInstance::EnableObCallback();
		}
		else if (tag == L"DISABLE_OBJ_REG_CB") {
			result = KernelInstance::DisableObCallback();
		}
		else if (tag == L"ENABLE_CM_REG_CB") {
			result = KernelInstance::EnableCmpCallback();
		}
		else if (tag == L"DISABLE_CM_REG_CB") {
			result = KernelInstance::DisableCmpCallback();
		}
		else if (tag == L"ENABLE_DSE") {
			result = KernelInstance::EnableDSE();
		}
		else if (tag == L"DISABLE_DSE") {
			result = KernelInstance::DisableDSE();
		}
		else if (tag == L"ENABLE_LKD") {
			result = KernelInstance::EnableLKD();
		}
		else if (tag == L"DISABLE_LKD") {
			result = KernelInstance::DisableLKD();
		}
		else if (tag == L"BSOD") {
			result = KernelInstance::BlueScreen(color);
		}
		else if (tag == L"PatchGuard") {
			result = KernelInstance::DisablePatchGuard(type);
		}
		else {
			co_await wil::resume_foreground(DispatcherQueue());
			slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Utility.Common.UnknownAction").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		if (result) {
			slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
		}
		else {
			if (GetLastError() == 0) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			}
			else {
				slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			}
		}

		if (hypervisor_mode) {
			ObjRegCbCard().Header(tbox(L"Utility.Header.ObjRegCbHVM"));
			DSECard().Header(tbox(L"Utility.Header.DSEHVM"));
		}

		co_return;
	}

	slg::coroutine UtilityPage::Button_Click2(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		auto button = sender.as<Button>();
		std::wstring tag = button.Tag().as<winrt::hstring>().c_str();

		co_await winrt::resume_background();

		LOG_INFO(L"UtilityPage", L"Confirmed we will do: %s", tag.c_str());

		BOOL result = FALSE;

		if (tag == L"POWER_SHUTDOWN") {
			result = KernelInstance::Shutdown();
		}
		else if (tag == L"POWER_REBOOT") {
			result = KernelInstance::Reboot();
		}
		else if (tag == L"POWER_REBOOT_FORCE") {
			result = KernelInstance::RebootForce();
		}
		else {
			slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Utility.Common.UnknownAction").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		if (result) {
			slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
		}
		else {
			slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
		}

		co_return;
	}

	void UtilityPage::SetupLocalization() {
		HVMCard().Header(tbox("Utility.Header.Card.HVM"));
		HVMCard().Description(tbox("Utility.Desc.Card.HVM"));
		CreateProcessCard().Header(tbox("Utility.Header.Card.CreateProcess"));
		CreateProcessCard().Description(tbox("Utility.Desc.Card.CreateProcess"));
		CreateFileCard().Header(tbox("Utility.Header.Card.CreateFile"));
		CreateFileCard().Description(tbox("Utility.Desc.Card.CreateFile"));
		LoadDrvCard().Header(tbox("Utility.Header.Card.LoadDrv"));
		LoadDrvCard().Description(tbox("Utility.Desc.Card.LoadDrv"));
		UnloadDrvCard().Header(tbox("Utility.Header.Card.UnloadDrv"));
		UnloadDrvCard().Description(tbox("Utility.Desc.Card.UnloadDrv"));
		ModifyRegCard().Header(tbox("Utility.Header.Card.ModifyReg"));
		ModifyRegCard().Description(tbox("Utility.Desc.Card.ModifyReg"));
		ModifyBootsecCard().Header(tbox("Utility.Header.Card.ModifyBootsec"));
		ModifyBootsecCard().Description(tbox("Utility.Desc.Card.ModifyBootsec"));
		ObjRegCbCard().Header(tbox("Utility.Header.Card.ObjRegCb"));
		ObjRegCbCard().Description(tbox("Utility.Desc.Card.ObjRegCb"));
		CmRegCbCard().Header(tbox("Utility.Header.Card.CmRegCb"));
		CmRegCbCard().Description(tbox("Utility.Desc.Card.CmRegCb"));
		DSECard().Header(tbox("Utility.Header.Card.DSE"));
		DSECard().Description(tbox("Utility.Desc.Card.DSE"));
		LKDCard().Header(tbox("Utility.Header.Card.LKD"));
		LKDCard().Description(tbox("Utility.Desc.Card.LKD"));
		PowerCard().Header(tbox("Utility.Header.Card.Power"));
		PowerCard().Description(tbox("Utility.Desc.Card.Power"));
		BSODCard().Header(tbox("Utility.Header.Card.BSOD"));
		BSODCard().Description(tbox("Utility.Desc.Card.BSOD"));
		PGCard().Header(tbox("Utility.Header.Card.PG"));
		PGCard().Description(tbox("Utility.Desc.Card.PG"));

		if (hypervisor_mode) {
			ObjRegCbCard().Header(tbox(L"Utility.Header.ObjRegCbHVM"));
			DSECard().Header(tbox(L"Utility.Header.DSEHVM"));
		}

		UtilityHypervisorUid().Text(t(L"Utility.Header.Hypervisor"));
		UtilityHVMEnableUid().Content(tbox(L"Utility.Menu.HVMEnable"));
		UtilitySysBehaviorUid().Text(t(L"Utility.Header.SysBehavior"));
		UtilitySysOpUid().Text(t(L"Utility.Header.SysOp"));
		UtilitySysOpWarningUid().Text(t(L"Utility.Msg.SysOpWarning"));
		UtilityPowerShutdownUid().Content(tbox(L"Utility.Menu.PowerShutdown"));
		UtilityPowerRebootUid().Content(tbox(L"Utility.Menu.PowerReboot"));
		UtilityPowerForceRebootUid().Content(tbox(L"Utility.Menu.PowerForceReboot"));
		UtilityBSODDefaultUid().Content(tbox(L"Utility.Menu.BSOD.Default"));
		UtilityBSODRedUid().Content(tbox(L"Utility.Menu.BSOD.Red"));
		UtilityBSODGreenUid().Content(tbox(L"Utility.Menu.BSOD.Green"));
		UtilityBSODBlueUid().Content(tbox(L"Utility.Menu.BSOD.Blue"));
		UtilityBSODYellowUid().Content(tbox(L"Utility.Menu.BSOD.Yellow"));
		UtilityBSODCyanUid().Content(tbox(L"Utility.Menu.BSOD.Cyan"));
		UtilityBSODMagentaUid().Content(tbox(L"Utility.Menu.BSOD.Magenta"));
		UtilityBSODBlackUid().Content(tbox(L"Utility.Menu.BSOD.Black"));
		UtilityBSODWhiteUid().Content(tbox(L"Utility.Menu.BSOD.White"));
		UtilityBSODOrangeUid().Content(tbox(L"Utility.Menu.BSOD.Orange"));
		UtilityBSODPurpleUid().Content(tbox(L"Utility.Menu.BSOD.Purple"));
		UtilityBSODPinkUid().Content(tbox(L"Utility.Menu.BSOD.Pink"));
		UtilityBSODGrayUid().Content(tbox(L"Utility.Menu.BSOD.Gray"));
		UtilityBSODBrownUid().Content(tbox(L"Utility.Menu.BSOD.Brown"));
		UtilityBSODGoldUid().Content(tbox(L"Utility.Menu.BSOD.Gold"));
		UtilityBSODSilverUid().Content(tbox(L"Utility.Menu.BSOD.Silver"));
		UtilityBSODCrashUid().Content(tbox(L"Utility.Menu.BSOD.Crash"));
		UtilityPGAutoUid().Content(tbox(L"Utility.Menu.PG.Auto"));
		UtilityPGDisableUid().Content(tbox(L"Utility.Menu.PG.Disable"));

		auto enableText = tbox(L"Utility.Menu.Enable");
		auto disableText = tbox(L"Utility.Menu.Disable");
		auto localizeButtons = [&](auto card) {
			auto panel = card.Content().as<winrt::Microsoft::UI::Xaml::Controls::StackPanel>();
			if (panel && panel.Children().Size() >= 2) {
				panel.Children().GetAt(0).as<Button>().Content(enableText);
				panel.Children().GetAt(1).as<Button>().Content(disableText);
			}
			};
		localizeButtons(CreateProcessCard());
		localizeButtons(CreateFileCard());
		localizeButtons(LoadDrvCard());
		localizeButtons(UnloadDrvCard());
		localizeButtons(ModifyRegCard());
		localizeButtons(ModifyBootsecCard());
		localizeButtons(ObjRegCbCard());
		localizeButtons(CmRegCbCard());
		localizeButtons(DSECard());
		localizeButtons(LKDCard());
	}
}
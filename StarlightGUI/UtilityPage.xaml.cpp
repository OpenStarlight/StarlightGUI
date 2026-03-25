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

		// SettingsCard Header+Description must be set in code (x:Uid on IInspectable DPs crashes).
		HVMCard().Header(tbox("Utility_HVM_Card.Header"));
		HVMCard().Description(tbox("Utility_HVM_Card.Description"));
		CreateProcessCard().Header(tbox("Utility_CreateProcess_Card.Header"));
		CreateProcessCard().Description(tbox("Utility_CreateProcess_Card.Description"));
		CreateFileCard().Header(tbox("Utility_CreateFile_Card.Header"));
		CreateFileCard().Description(tbox("Utility_CreateFile_Card.Description"));
		LoadDrvCard().Header(tbox("Utility_LoadDrv_Card.Header"));
		LoadDrvCard().Description(tbox("Utility_LoadDrv_Card.Description"));
		UnloadDrvCard().Header(tbox("Utility_UnloadDrv_Card.Header"));
		UnloadDrvCard().Description(tbox("Utility_UnloadDrv_Card.Description"));
		ModifyRegCard().Header(tbox("Utility_ModifyReg_Card.Header"));
		ModifyRegCard().Description(tbox("Utility_ModifyReg_Card.Description"));
		ModifyBootsecCard().Header(tbox("Utility_ModifyBootsec_Card.Header"));
		ModifyBootsecCard().Description(tbox("Utility_ModifyBootsec_Card.Description"));
		ObjRegCbCard().Header(tbox("Utility_ObjRegCb_Card.Header"));
		ObjRegCbCard().Description(tbox("Utility_ObjRegCb_Card.Description"));
		CmRegCbCard().Header(tbox("Utility_CmRegCb_Card.Header"));
		CmRegCbCard().Description(tbox("Utility_CmRegCb_Card.Description"));
		DSECard().Header(tbox("Utility_DSE_Card.Header"));
		DSECard().Description(tbox("Utility_DSE_Card.Description"));
		LKDCard().Header(tbox("Utility_LKD_Card.Header"));
		LKDCard().Description(tbox("Utility_LKD_Card.Description"));
		PowerCard().Header(tbox("Utility_Power_Card.Header"));
		PowerCard().Description(tbox("Utility_Power_Card.Description"));
		BSODCard().Header(tbox("Utility_BSOD_Card.Header"));
		BSODCard().Description(tbox("Utility_BSOD_Card.Description"));
		PGCard().Header(tbox("Utility_PG_Card.Header"));
		PGCard().Description(tbox("Utility_PG_Card.Description"));

		if (hypervisor_mode) {
			ObjRegCbCard().Header(tbox(L"Utility_ObjRegCb_HVM"));
			DSECard().Header(tbox(L"Utility_DSE_HVM"));
		}

		// Localize section headers and buttons
		UtilityHypervisorUid().Text(GetLocalizedString(L"Utility_Hypervisor.Text"));
		UtilityHVMEnableUid().Content(tbox(L"Utility_HVM_Enable.Content"));
		UtilitySysBehaviorUid().Text(GetLocalizedString(L"Utility_SysBehavior.Text"));
		UtilitySysOpUid().Text(GetLocalizedString(L"Utility_SysOp.Text"));
		UtilitySysOpWarningUid().Text(GetLocalizedString(L"Utility_SysOpWarning.Text"));
		UtilityPowerShutdownUid().Content(tbox(L"Utility_PowerShutdown.Content"));
		UtilityPowerRebootUid().Content(tbox(L"Utility_PowerReboot.Content"));
		UtilityPowerForceRebootUid().Content(tbox(L"Utility_PowerForceReboot.Content"));
		UtilityBSODDefaultUid().Content(tbox(L"Utility_BSOD_Default.Content"));
		UtilityBSODRedUid().Content(tbox(L"Utility_BSOD_Red.Content"));
		UtilityBSODGreenUid().Content(tbox(L"Utility_BSOD_Green.Content"));
		UtilityBSODBlueUid().Content(tbox(L"Utility_BSOD_Blue.Content"));
		UtilityBSODYellowUid().Content(tbox(L"Utility_BSOD_Yellow.Content"));
		UtilityBSODCyanUid().Content(tbox(L"Utility_BSOD_Cyan.Content"));
		UtilityBSODMagentaUid().Content(tbox(L"Utility_BSOD_Magenta.Content"));
		UtilityBSODBlackUid().Content(tbox(L"Utility_BSOD_Black.Content"));
		UtilityBSODWhiteUid().Content(tbox(L"Utility_BSOD_White.Content"));
		UtilityBSODOrangeUid().Content(tbox(L"Utility_BSOD_Orange.Content"));
		UtilityBSODPurpleUid().Content(tbox(L"Utility_BSOD_Purple.Content"));
		UtilityBSODPinkUid().Content(tbox(L"Utility_BSOD_Pink.Content"));
		UtilityBSODGrayUid().Content(tbox(L"Utility_BSOD_Gray.Content"));
		UtilityBSODBrownUid().Content(tbox(L"Utility_BSOD_Brown.Content"));
		UtilityBSODGoldUid().Content(tbox(L"Utility_BSOD_Gold.Content"));
		UtilityBSODSilverUid().Content(tbox(L"Utility_BSOD_Silver.Content"));
		UtilityBSODCrashUid().Content(tbox(L"Utility_BSOD_Crash.Content"));
		UtilityPGAutoUid().Content(tbox(L"Utility_PG_Auto.Content"));
		UtilityPGDisableUid().Content(tbox(L"Utility_PG_Disable.Content"));

		// Localize all Enable/Disable buttons (they share Tag-based handlers, no x:Name)
		auto enableText = tbox(L"Utility_Enable.Content");
		auto disableText = tbox(L"Utility_Disable.Content");
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

		LOG_INFO(L"UtilityPage", L"UtilityPage initialized.");
	}

	slg::coroutine UtilityPage::Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		auto button = sender.as<Button>();
		std::wstring tag = button.Tag().as<winrt::hstring>().c_str();

		if (safeAcceptedTag != tag) {
			safeAcceptedTag = tag;
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Utility_ConfirmAction").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Utility_ConfirmAction2").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
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
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Utility_UnknownAction").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		if (result) {
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Utility_ActionSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
		}
		else {
			if (GetLastError() == 0) {
				slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Utility_ActionFailedAlready").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			}
			else {
				slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Utility_ActionFailed").c_str() + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			}
		}

		if (hypervisor_mode) {
			ObjRegCbCard().Header(tbox(L"Utility_ObjRegCb_HVM"));
			DSECard().Header(tbox(L"Utility_DSE_HVM"));
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
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Utility_UnknownAction").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		if (result) {
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Utility_ActionSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
		}
		else {
			slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Utility_ActionFailed").c_str() + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
		}

		co_return;
	}
}





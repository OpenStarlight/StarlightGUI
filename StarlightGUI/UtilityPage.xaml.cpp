#include "pch.h"
#include "UtilityPage.xaml.h"
#if __has_include("UtilityPage.g.cpp")
#include "UtilityPage.g.cpp"
#endif

#include "LoadDriverDialog.xaml.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation {
	static hstring GetDriverErrorMessage()
	{
		auto errorMsg = KernelInstance::GetLastErrorMessage();
		if (!errorMsg.empty()) {
			return t(L"Msg.DriverError.Detail", errorMsg.c_str());
		}

		auto errorCode = KernelInstance::GetLastErrorCode();
		if (errorCode == 0) {
			return t(L"Msg.Failed", GetLastError());
		}

		wchar_t hexCode[32];
		swprintf_s(hexCode, L"0x%X", errorCode);
		return t(L"Msg.DriverError.Code", hexCode);
	}

	UtilityPage::UtilityPage() {
		InitializeComponent();
		SetupLocalization();

#ifndef STARLIGHT_PREMIUM
		PGCard().Opacity(0.45);
		HypervisorCard().Opacity(0.45);
		DSEHypervisorCard().Opacity(0.45);
		PGHypervisorCard().Opacity(0.45);
		LoadDrvHypervisorCard().Opacity(0.45);
		ToolTipService::SetToolTip(PGCard(), tbox(L"Common.PremiumOnly"));
		ToolTipService::SetToolTip(HypervisorCard(), tbox(L"Common.PremiumOnly"));
		ToolTipService::SetToolTip(DSEHypervisorCard(), tbox(L"Common.PremiumOnly"));
		ToolTipService::SetToolTip(PGHypervisorCard(), tbox(L"Common.PremiumOnly"));
		ToolTipService::SetToolTip(LoadDrvHypervisorCard(), tbox(L"Common.PremiumOnly"));
#endif

		LOG_INFO(L"UtilityPage", L"UtilityPage initialized.");
	}

	slg::coroutine UtilityPage::Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		auto button = sender.as<Button>();
		std::wstring tag = button.Tag().as<winrt::hstring>().c_str();

		if (dangerous_confirm && !(co_await slg::ShowConfirmDialog(t(L"Common.Warning"), t(L"Utility.Msg.ConfirmAction"), t(L"Common.Continue"), t(L"Common.Cancel"), XamlRoot()))) {
			co_return;
		}

		co_await winrt::resume_background();

		BOOL result = FALSE;

		if (tag == L"ENABLE_HYPERVISOR") {
			result = KernelInstance::EnableHypervisor();
			hypervisor_mode = result;
		}
		else if (tag == L"DISABLE_HYPERVISOR") {
			result = KernelInstance::DisableHypervisor();
			if (result) {
				hypervisor_mode = false;
			}
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
		else if (tag == L"ENABLE_MODIFY_REG") {
			result = KernelInstance::EnableModifyRegistry();
		}
		else if (tag == L"DISABLE_MODIFY_REG") {
			result = KernelInstance::DisableModifyRegistry();
		}
		else if (tag == L"ENABLE_DSE") {
			result = KernelInstance::EnableDSE(false);
		}
		else if (tag == L"DISABLE_DSE") {
			result = KernelInstance::DisableDSE(false);
		}
		else if (tag == L"ENABLE_DSE_HYPERVISOR") {
			result = KernelInstance::EnableDSE(true);
		}
		else if (tag == L"DISABLE_DSE_HYPERVISOR") {
			result = KernelInstance::DisableDSE(true);
		}
		else if (tag == L"ENABLE_LKD") {
			result = KernelInstance::EnableLKD();
		}
		else if (tag == L"BSOD") {
			result = KernelInstance::BlueScreen();
		}
		else if (tag == L"PATCHGUARD") {
			result = KernelInstance::DisablePatchGuard(false);
		}
		else if (tag == L"PATCHGUARD_HYPERVISOR") {
			result = KernelInstance::DisablePatchGuard(true);
		}
		else {
			co_await wil::resume_foreground(DispatcherQueue());
			slg::CreateInfoBarAndDisplay(t(L"Common.Error"), t(L"Utility.Common.UnknownAction").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
		}

		auto errorMessage = GetDriverErrorMessage();
		co_await wil::resume_foreground(DispatcherQueue());

		if (result) {
			slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
		}
		else {
			slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), errorMessage.c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
		}

		co_return;
	}

	slg::coroutine UtilityPage::LoadDriverHypervisorButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
#ifdef STARLIGHT_PREMIUM
		try {
			auto dialog = winrt::make<winrt::StarlightGUI::implementation::LoadDriverDialog>();
			dialog.XamlRoot(this->XamlRoot());

			auto result = co_await dialog.ShowAsync();
			if (result != ContentDialogResult::Primary) {
				co_return;
			}

			hstring driverPath = dialog.DriverPath();
			bool bypass = dialog.Bypass();

			auto file = co_await winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(driverPath);
			hstring fileName = file.Name();

			co_await winrt::resume_background();

			BOOL dseDisabled = TRUE;
			if (bypass) {
				LOG_WARNING(__WFUNCTION__, L"Bypass flag enabled! Disabling DSE by hypervisor...");
				dseDisabled = KernelInstance::DisableDSE(true);
			}

			bool status = dseDisabled && DriverUtils::LoadDriver(driverPath.c_str(), fileName.c_str());
			auto driverError = GetDriverErrorMessage();
			auto win32Error = GetLastError();

			if (bypass && dseDisabled) {
				KernelInstance::EnableDSE(true);
			}

			co_await wil::resume_foreground(DispatcherQueue());

			if (status) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else if (!dseDisabled) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), driverError.c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
			}
			else {
				slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", win32Error), InfoBarSeverity::Error, g_mainWindowInstance);
			}
		}
		catch (winrt::hresult_error const& ex) {
			slg::CreateInfoBarAndDisplay(t(L"Common.Error"), (t(L"Msg.ShowDialog.Failed") + ex.message()).c_str(),
				InfoBarSeverity::Error, g_mainWindowInstance);
		}
		co_return;
#else
		KernelInstance::DisableHypervisor(); // 随便
		slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
		co_return;
#endif
	}

	void UtilityPage::SetupLocalization() {
		CreateProcessCard().Header(tbox("Utility.Header.Card.CreateProcess"));
		CreateProcessCard().Description(tbox("Utility.Desc.Card.CreateProcess"));
		CreateFileCard().Header(tbox("Utility.Header.Card.CreateFile"));
		CreateFileCard().Description(tbox("Utility.Desc.Card.CreateFile"));
		ModifyRegCard().Header(tbox("Utility.Header.Card.ModifyReg"));
		ModifyRegCard().Description(tbox("Utility.Desc.Card.ModifyReg"));
		DSECard().Header(tbox("Utility.Header.Card.DSE"));
		DSECard().Description(tbox("Utility.Desc.Card.DSE"));
		LKDCard().Header(tbox("Utility.Header.Card.LKD"));
		LKDCard().Description(tbox("Utility.Desc.Card.LKD"));
		BSODCard().Header(tbox("Utility.Header.Card.BSOD"));
		BSODCard().Description(tbox("Utility.Desc.Card.BSOD"));
		PGCard().Header(tbox("Utility.Header.Card.PG"));
		PGCard().Description(tbox("Utility.Desc.Card.PG"));
		HypervisorCard().Header(tbox("Utility.Header.Card.Hypervisor"));
		HypervisorCard().Description(tbox("Utility.Desc.Card.Hypervisor"));
		LoadDrvHypervisorCard().Header(tbox("Utility.Header.Card.LoadDrvHypervisor"));
		LoadDrvHypervisorCard().Description(tbox("Utility.Desc.Card.LoadDrvHypervisor"));
		DSEHypervisorCard().Header(tbox("Utility.Header.Card.DSEHypervisor"));
		DSEHypervisorCard().Description(tbox("Utility.Desc.Card.DSEHypervisor"));
		PGHypervisorCard().Header(tbox("Utility.Header.Card.PGHypervisor"));
		PGHypervisorCard().Description(tbox("Utility.Desc.Card.PGHypervisor"));

		UtilitySysBehaviorUid().Text(t(L"Utility.Header.SysBehavior"));
		UtilityHypervisorUid().Text(t(L"Utility.Header.Hypervisor"));
		UtilityHypervisorEnableUid().Content(tbox(L"Utility.Menu.HypervisorEnable"));
		UtilityHypervisorDisableUid().Content(tbox(L"Utility.Menu.HypervisorDisable"));
		UtilityLKDEnableUid().Content(tbox(L"Utility.Menu.Enable"));
		UtilityBSODCrashUid().Content(tbox(L"Utility.Menu.BSOD.Crash"));
		UtilityPGDisableUid().Content(tbox(L"Utility.Menu.PG.Disable"));
		UtilityPGHypervisorDisableUid().Content(tbox(L"Utility.Menu.PG.Disable"));
		UtilityLoadDriverHypervisorUid().Content(tbox(L"KernelModule.Button.LoadDriver"));

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
		localizeButtons(ModifyRegCard());
		localizeButtons(DSECard());
		localizeButtons(DSEHypervisorCard());
	}
}

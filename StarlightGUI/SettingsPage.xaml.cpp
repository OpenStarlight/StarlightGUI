#include "pch.h"
#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

#include "Utils/Config.h"
#include "MainWindow.xaml.h"
#include <algorithm>
#include <cwctype>

using namespace winrt;
using namespace Windows::Storage;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::Windows::Storage::Pickers;

namespace winrt::StarlightGUI::implementation
{
    static bool loaded;
    static bool autoStartChanging;
    static bool replaceTaskManagerChanging;
    static const std::wstring autoStartTaskName = L"StarlightGUI_AutoStart";
    static const std::wstring replaceTaskManagerRegPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe";
    static const std::wstring replaceTaskManagerTaskName = L"StarlightGUI_OpenTaskMgr";
    static const std::wstring replaceTaskManagerTriggerScriptName = L"StarlightGUI_OpenTaskMgr.vbs";
    static const std::wstring replaceTaskManagerLaunchScriptName = L"StarlightGUI_OpenTaskMgrLaunch.vbs";

    static std::wstring GetConfigSidePath(std::wstring const& fileName) { return GetInstalledLocationPath() + L"\\" + fileName; }

    static bool UpdateAutoStartTask(bool enabled)
    {
        if (enabled) {
            std::wstring action = L"\\\"" + GetExecutablePath() + L"\\\"";
            return RunSchtasks(L"/Create /TN \"" + autoStartTaskName + L"\" /TR \"" + action + L"\" /SC ONLOGON /RL HIGHEST /F");
        }
        if (!QueryTaskExists(autoStartTaskName)) return true;
        return RunSchtasks(L"/Delete /TN \"" + autoStartTaskName + L"\" /F");
    }

    static std::wstring GetReplaceTaskManagerCommand()
    {
        return L"\"" + GetSystemToolPath(L"wscript.exe") + L"\" //B \"" + GetConfigSidePath(replaceTaskManagerTriggerScriptName) + L"\"";
    }

    static bool ReplaceTaskManagerTaskExists()
    {
        return QueryTaskExists(replaceTaskManagerTaskName);
    }

    static bool CreateReplaceTaskManagerTask()
    {
        std::wstring action = L"\\\"" + GetSystemToolPath(L"wscript.exe") + L"\\\" //B \\\"" + GetConfigSidePath(replaceTaskManagerLaunchScriptName) + L"\\\"";
        if (!RunSchtasks(L"/Create /TN \"" + replaceTaskManagerTaskName + L"\" /TR \"" + action + L"\" /SC ONCE /ST 00:00 /RL HIGHEST /F")) return false;
        return ReplaceTaskManagerTaskExists();
    }

    static bool DeleteReplaceTaskManagerTask()
    {
        if (!ReplaceTaskManagerTaskExists()) return true;
        return RunSchtasks(L"/Delete /TN \"" + replaceTaskManagerTaskName + L"\" /F");
    }

    static bool EnsureReplaceTaskManagerScript()
    {
        std::string triggerScript = "Set shell = CreateObject(\"WScript.Shell\")\r\n";
        triggerScript += "shell.Run Chr(34) & \"";
        triggerScript += WideStringToString(GetSystemToolPath(L"schtasks.exe"));
        triggerScript += "\" & Chr(34) & \" /Run /TN \" & Chr(34) & \"StarlightGUI_OpenTaskMgr\" & Chr(34), 0, False\r\n";

        std::string launchScript = "Set shell = CreateObject(\"Shell.Application\")\r\n";
        launchScript += "shell.ShellExecute \"";
        launchScript += WideStringToString(GetExecutablePath());
        launchScript += "\", \"--open-taskmgr\", \"\", \"open\", 1\r\n";

        return WriteTextFile(GetConfigSidePath(replaceTaskManagerTriggerScriptName), triggerScript)
            && WriteTextFile(GetConfigSidePath(replaceTaskManagerLaunchScriptName), launchScript);
    }

    static bool DeleteReplaceTaskManagerScript()
    {
        bool success = true;
        std::error_code ec;
        auto triggerScriptPath = GetConfigSidePath(replaceTaskManagerTriggerScriptName);
        auto launchScriptPath = GetConfigSidePath(replaceTaskManagerLaunchScriptName);

        if (std::filesystem::exists(triggerScriptPath)) {
            std::filesystem::remove(triggerScriptPath, ec);
            if (ec) success = false;
            ec.clear();
        }

        if (std::filesystem::exists(launchScriptPath)) {
            std::filesystem::remove(launchScriptPath, ec);
            if (ec) success = false;
        }

        return success;
    }

    static bool IsTaskManagerReplaced()
    {
        HKEY hKey = NULL;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, replaceTaskManagerRegPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return false;
        }

        wchar_t value[2048] = {};
        DWORD type = REG_SZ;
        DWORD size = sizeof(value);
        auto result = RegQueryValueExW(hKey, L"Debugger", nullptr, &type, (LPBYTE)value, &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS || type != REG_SZ) return false;

        return CompareIgnoreCase(value, GetReplaceTaskManagerCommand()) == 0
            && ReplaceTaskManagerTaskExists()
            && std::filesystem::exists(GetConfigSidePath(replaceTaskManagerTriggerScriptName))
            && std::filesystem::exists(GetConfigSidePath(replaceTaskManagerLaunchScriptName));
    }

    static bool SetTaskManagerReplaceState(bool enabled)
    {
        HKEY hKey = NULL;
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, replaceTaskManagerRegPath.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
            return false;
        }

        bool success = false;
        if (enabled) {
            if (CreateReplaceTaskManagerTask() && EnsureReplaceTaskManagerScript()) {
                auto command = GetReplaceTaskManagerCommand();
                success = RegSetValueExW(hKey, L"Debugger", 0, REG_SZ, (const BYTE*)command.c_str(), (DWORD)((command.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
            }
        }
        else {
            auto result = RegDeleteValueW(hKey, L"Debugger");
            success = (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
            success = success && DeleteReplaceTaskManagerScript() && DeleteReplaceTaskManagerTask();
        }

        RegCloseKey(hKey);
        return success;
    }

    SettingsPage::SettingsPage()
    {
        InitializeComponent();
        InitializeOptions();
    }

    void SettingsPage::InitializeOptions() {
        EnumFileModeComboBox().SelectedIndex(enum_file_mode);
        BackgroundComboBox().SelectedIndex(background_type);
        NavigationComboBox().SelectedIndex(navigation_style);
        MicaTypeComboBox().SelectedIndex(mica_type);
        AcrylicTypeComboBox().SelectedIndex(acrylic_type);
        ImageStretchComboBox().SelectedIndex(image_stretch);

        EnumStrengthenButton().IsOn(enum_strengthen);
        PDHFirstButton().IsOn(pdh_first);
		ElevatedRunButton().IsOn(elevated_run);
        DangerousConfirmButton().IsOn(dangerous_confirm);
        CheckUpdateButton().IsOn(check_update);
        TaskAutoRefreshButton().IsOn(task_auto_refresh);
        TrayBackgroundRunButton().IsOn(tray_background_run);

        auto_start = QueryTaskExists(autoStartTaskName);
        SaveConfig("auto_start", auto_start);
        AutoStartButton().IsOn(auto_start);

        replace_taskmgr = IsTaskManagerReplaced();
        SaveConfig("replace_taskmgr", replace_taskmgr);
        ReplaceTaskManagerButton().IsOn(replace_taskmgr);

        ImagePathText().Text(to_hstring(background_image));
        ImageOpacitySlider().Value(image_opacity);
		DisasmCountSlider().Value(disasm_count);
        LanguageComboBox().SelectedIndex((language == "zh-CN") ? 1 : (language == "en-US") ? 2 : 0);

        SettingsFeatureUid().Text(t(L"Settings_Feature.Text"));
        SettingsMainUid().Text(t(L"Settings_Main.Text"));
        SettingsAppearanceUid().Text(t(L"Settings_Appearance.Text"));
        SettingsImportImageUid().Content(tbox(L"Settings_ImportImage.Content"));
        SettingsClearImageUid().Content(tbox(L"Settings_ClearImage.Content"));
        SettingsImageRefreshUid().Content(tbox(L"Settings_ImageRefresh.Content"));
        SettingsOtherUid().Text(t(L"Settings_Other.Text"));
        SettingsLogUid().Content(tbox(L"Settings_Log.Content"));
        SettingsFixUid().Content(tbox(L"Settings_Fix.Content"));
        EnumFileModeCard().Header(tbox("Settings_EnumFileMode_Card.Header"));
        EnumFileModeCard().Description(tbox("Settings_EnumFileMode_Card.Description"));
        EnumStrengthenCard().Header(tbox("Settings_EnumStrengthen_Card.Header"));
        EnumStrengthenCard().Description(tbox("Settings_EnumStrengthen_Card.Description"));
        TaskAutoRefreshCard().Header(tbox("Settings_TaskAutoRefresh_Card.Header"));
        TaskAutoRefreshCard().Description(tbox("Settings_TaskAutoRefresh_Card.Description"));
        PDHFirstCard().Header(tbox("Settings_PDHFirst_Card.Header"));
        PDHFirstCard().Description(tbox("Settings_PDHFirst_Card.Description"));
        DisasmCountCard().Header(tbox("Settings_DisasmCount_Card.Header"));
        DisasmCountCard().Description(tbox("Settings_DisasmCount_Card.Description"));
        TrayBgRunCard().Header(tbox("Settings_TrayBgRun_Card.Header"));
        TrayBgRunCard().Description(tbox("Settings_TrayBgRun_Card.Description"));
        ElevatedRunCard().Header(tbox("Settings_ElevatedRun_Card.Header"));
        ElevatedRunCard().Description(tbox("Settings_ElevatedRun_Card.Description"));
        AutoStartCard().Header(tbox("Settings_AutoStart_Card.Header"));
        AutoStartCard().Description(tbox("Settings_AutoStart_Card.Description"));
        ReplaceTaskMgrCard().Header(tbox("Settings_ReplaceTaskMgr_Card.Header"));
        ReplaceTaskMgrCard().Description(tbox("Settings_ReplaceTaskMgr_Card.Description"));
        BackgroundExpander().Header(tbox("Settings_Background_Card.Header"));
        BackgroundExpander().Description(tbox("Settings_Background_Card.Description"));
        AcrylicTypeCard().Header(tbox("Settings_AcrylicType_Card.Header"));
        AcrylicTypeCard().Description(tbox("Settings_AcrylicType_Card.Description"));
        MicaTypeCard().Header(tbox("Settings_MicaType_Card.Header"));
        MicaTypeCard().Description(tbox("Settings_MicaType_Card.Description"));
        ImageBackgroundExpander().Header(tbox("Settings_ImageBackground_Card.Header"));
        ImageBackgroundExpander().Description(tbox("Settings_ImageBackground_Card.Description"));
        ImportImageCard().Header(tbox("Settings_ImportImage_Card.Header"));
        ImportImageCard().Description(tbox("Settings_ImportImage_Card.Description"));
        ClearImageCard().Header(tbox("Settings_ClearImage_Card.Header"));
        ClearImageCard().Description(tbox("Settings_ClearImage_Card.Description"));
        ImageStretchCard().Header(tbox("Settings_ImageStretch_Card.Header"));
        ImageStretchCard().Description(tbox("Settings_ImageStretch_Card.Description"));
        ImageOpacityCard().Header(tbox("Settings_ImageOpacity_Card.Header"));
        ImageOpacityCard().Description(tbox("Settings_ImageOpacity_Card.Description"));
        NavigationCard().Header(tbox("Settings_Navigation_Card.Header"));
        NavigationCard().Description(tbox("Settings_Navigation_Card.Description"));
        LanguageCard().Header(tbox("Settings_LanguageCard.Header"));
        LanguageCard().Description(tbox("Settings_LanguageCard.Description"));
        DangerousConfirmCard().Header(tbox("Settings_DangerousConfirm_Card.Header"));
        DangerousConfirmCard().Description(tbox("Settings_DangerousConfirm_Card.Description"));
        CheckUpdateCard().Header(tbox("Settings_CheckUpdate_Card.Header"));
        CheckUpdateCard().Description(tbox("Settings_CheckUpdate_Card.Description"));
        LogCard().Header(tbox("Settings_Log_Card.Header"));
        LogCard().Description(tbox("Settings_Log_Card.Description"));
        FixCard().Header(tbox("Settings_Fix_Card.Header"));
        FixCard().Description(tbox("Settings_Fix_Card.Description"));
    }

    void SettingsPage::EnumFileModeComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        enum_file_mode = (int)EnumFileModeComboBox().SelectedIndex();
        SaveConfig("enum_file_mode", (int)EnumFileModeComboBox().SelectedIndex());
    }

    void SettingsPage::EnumStrengthenButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) return;
		enum_strengthen = EnumStrengthenButton().IsOn();
        SaveConfig("enum_strengthen", enum_strengthen);
    }

    void SettingsPage::PDHFirstButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) return;
        pdh_first = PDHFirstButton().IsOn();
        SaveConfig("pdh_first", pdh_first);
    }

    void SettingsPage::BackgroundComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        background_type = (int)BackgroundComboBox().SelectedIndex();
        SaveConfig("background_type", (int)BackgroundComboBox().SelectedIndex());

        g_mainWindowInstance->LoadBackdrop();

        Console::GetInstance().SetBackdropByConfig();
    }

    void SettingsPage::MicaTypeComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        mica_type = (int)MicaTypeComboBox().SelectedIndex();

        SaveConfig("mica_type", (int)MicaTypeComboBox().SelectedIndex());

        g_mainWindowInstance->LoadBackdrop();
    }

    void SettingsPage::AcrylicTypeComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        acrylic_type = (int)AcrylicTypeComboBox().SelectedIndex();

        SaveConfig("acrylic_type", (int)AcrylicTypeComboBox().SelectedIndex());

        g_mainWindowInstance->LoadBackdrop();
    }
    
    void SettingsPage::NavigationComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        navigation_style = (int)NavigationComboBox().SelectedIndex();
        SaveConfig("navigation_style", (int)NavigationComboBox().SelectedIndex());

        g_mainWindowInstance->LoadNavigation();
    }

    void SettingsPage::ElevatedRunButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"Msg_RestartRequired").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
        elevated_run = ElevatedRunButton().IsOn();
        SaveConfig("elevated_run", elevated_run);
    }

    void SettingsPage::DangerousConfirmButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        dangerous_confirm = DangerousConfirmButton().IsOn();
        SaveConfig("dangerous_confirm", dangerous_confirm);
    }

    void SettingsPage::CheckUpdateButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
		slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"Msg_RestartRequired").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
        check_update = CheckUpdateButton().IsOn();
        SaveConfig("check_update", check_update);
    }

    void SettingsPage::TaskAutoRefreshButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        task_auto_refresh = TaskAutoRefreshButton().IsOn();
        SaveConfig("task_auto_refresh", task_auto_refresh);
    }

    void SettingsPage::TrayBackgroundRunButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        tray_background_run = TrayBackgroundRunButton().IsOn();
        SaveConfig("tray_background_run", tray_background_run);
        g_mainWindowInstance->SetTrayBackgroundRun(tray_background_run);
    }

    void SettingsPage::AutoStartButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (autoStartChanging) return;

        bool enabled = AutoStartButton().IsOn();

        if (enabled) {
            if (UpdateAutoStartTask(true)) {
                auto_start = true;
                SaveConfig("auto_start", true);
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg_AutoStartEnabled").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else {
                autoStartChanging = true;
                AutoStartButton().IsOn(false);
                autoStartChanging = false;

                auto_start = false;
                SaveConfig("auto_start", false);
                slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg_AutoStartEnableFailed").c_str(), InfoBarSeverity::Error, g_mainWindowInstance, 2500);
            }
        }
        else {
            if (UpdateAutoStartTask(false)) {
                auto_start = false;
                SaveConfig("auto_start", false);
                slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg_AutoStartDisabled").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else {
                autoStartChanging = true;
                AutoStartButton().IsOn(true);
                autoStartChanging = false;

                auto_start = true;
                SaveConfig("auto_start", true);
                slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg_AutoStartDisableFailed").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
    }

    void SettingsPage::ReplaceTaskManagerButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (replaceTaskManagerChanging) return;

        bool enabled = ReplaceTaskManagerButton().IsOn();
        if (SetTaskManagerReplaceState(enabled)) {
            replace_taskmgr = enabled;
            SaveConfig("replace_taskmgr", replace_taskmgr);
            auto msg = enabled ? t(L"Msg_ReplaceTaskMgrEnabled") : t(L"Msg_ReplaceTaskMgrDisabled");
            slg::CreateInfoBarAndDisplay(t(L"Common.Success"), msg.c_str(), InfoBarSeverity::Success, g_mainWindowInstance, 2500);
        }
        else {
            replaceTaskManagerChanging = true;
            ReplaceTaskManagerButton().IsOn(!enabled);
            replaceTaskManagerChanging = false;

            replace_taskmgr = !enabled;
            SaveConfig("replace_taskmgr", replace_taskmgr);
            slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg_ReplaceTaskMgrFailed").c_str(), InfoBarSeverity::Error, g_mainWindowInstance, 2500);
        }
    }

    void SettingsPage::ClearImageButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) return;
        SaveConfig("background_image", "");
        ImagePathText().Text(L"");

        g_mainWindowInstance->LoadBackground();
    }

    slg::coroutine SettingsPage::SetImageButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) co_return;
        HWND hWnd = g_mainWindowInstance->GetWindowHandle();

        FileOpenPicker picker = FileOpenPicker(winrt::Microsoft::UI::GetWindowIdFromWindow(hWnd));

        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().Append(L".png");
        picker.FileTypeFilter().Append(L".jpg");
        picker.FileTypeFilter().Append(L".bmp");
        picker.FileTypeFilter().Append(L".jpeg");

        auto result = co_await picker.PickSingleFileAsync();

        if (!result) co_return;

        try {
            auto file = co_await StorageFile::GetFileFromPathAsync(result.Path());

            if (file && file.IsAvailable() && (file.FileType() == L".png" || file.FileType() == L".jpg" || file.FileType() == L".bmp" || file.FileType() == L".jpeg")) {
                std::string path = WideStringToString(file.Path().c_str());
                SaveConfig("background_image", path);
                ImagePathText().Text(to_hstring(background_image));

                g_mainWindowInstance->LoadBackground();
            }
        }
        catch (hresult_error) {

        }
    }


    void SettingsPage::RefreshOpacityButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) return;
        g_mainWindowInstance->LoadBackground();
    }

    void SettingsPage::ImageStretchComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        image_stretch = (int)ImageStretchComboBox().SelectedIndex();
        SaveConfig("image_stretch", (int)ImageStretchComboBox().SelectedIndex());

        g_mainWindowInstance->LoadBackground();
    }

    void SettingsPage::LogButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        LOGGER_TOGGLE();
    }

    void SettingsPage::FixButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"Msg_RestartRequired").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
        DriverUtils::FixServices();
    }

    void SettingsPage::ImageOpacitySlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        SaveConfig("image_opacity", ImageOpacitySlider().Value());
    }

    void SettingsPage::DisasmCountSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        SaveConfig("disasm_count", DisasmCountSlider().Value());
    }

    void SettingsPage::LanguageComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        if (slg::CheckIllegalComboBoxAction(sender, e)) return;

        int idx = (int)LanguageComboBox().SelectedIndex();
        std::string lang = (idx == 0) ? "system" : (idx == 1) ? "zh-CN" : "en-US";
        SaveConfig("language", lang);
        slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"Msg_RestartRequired").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
    }
}





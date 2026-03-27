#include "pch.h"
#include "HelpPage.xaml.h"
#if __has_include("HelpPage.g.cpp")
#include "HelpPage.g.cpp"
#endif

#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Data.Json.h>
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Web::Http;
using namespace Windows::Foundation;
using namespace Windows::Data::Json;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    HelpPage::HelpPage() {
        InitializeComponent();

        this->Loaded([this](auto&&, auto&&) -> winrt::Windows::Foundation::IAsyncAction {
            auto weak_this = get_weak();
            if (sponsorList.empty()) {
                co_await GetSponsorListFromCloud();
            }
            if (auto strong_this = weak_this.get()) {
                SetSponsorList();
            }
            });

        LOG_INFO(L"HelpPage", L"HelpPage initialized.");
    }

    void HelpPage::GithubButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://github.com/OpenStarlight/StarlightGUI");
    }

    void HelpPage::Github2Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://github.com/HO-COOH/WinUIEssentials");
    }

    void HelpPage::GithubUserButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://github.com/RinoRika");
    }

    void HelpPage::GithubUser2Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://github.com/HO-COOH");
    }

    void HelpPage::GithubUser3Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://github.com/PspExitThread");
    }

    void HelpPage::GithubUser4Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://github.com/MuLin4396");
    }

    void HelpPage::BilibiliButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://space.bilibili.com/670866766");
    }

    void HelpPage::Bilibili2Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://space.bilibili.com/3494361276877525");
    }

    void HelpPage::SponsorButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://afdian.com/a/StarsAzusa");
    }

    void HelpPage::WinUIButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        LaunchURI(L"https://aka.ms/windev");
    }

    slg::coroutine HelpPage::LaunchURI(hstring uri) {
        Uri target(uri);
        LOG_INFO(__WFUNCTION__, L"Launching URI link: %s", uri.c_str());
        auto result = co_await Launcher::LaunchUriAsync(target);

        if (result) {
            slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
        }
        else {
            slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
        }
    }

    winrt::Windows::Foundation::IAsyncAction HelpPage::GetSponsorListFromCloud() {
        try {
            auto weak_this = get_weak();

            if (auto strong_this = weak_this.get()) {
                co_await winrt::resume_background();

                HttpClient client;
                Uri uri(L"https://pastebin.com/raw/vVhAkyVT");

                // 防止获取旧数据
                client.DefaultRequestHeaders().Append(L"Cache-Control", L"no-cache");
                client.DefaultRequestHeaders().Append(L"If-None-Match", L"");

                LOG_INFO(L"Updater", L"Getting sponsor list...");
                hstring result = co_await client.GetStringAsync(uri);

                auto json = Windows::Data::Json::JsonObject::Parse(result);
                hstring list = json.GetNamedString(L"sponsors");

                sponsorList = list;
            }
        }
        catch (const hresult_error& e) {
            LOG_ERROR(__WFUNCTION__, L"Failed to get sponsor list! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
            sponsorList = t(L"Msg.FetchFailed");
        }
        co_return;
    }

    void HelpPage::SetSponsorList() {
        if (sponsorList.empty()) {
            SponsorListText().Text(t(L"Msg.FetchFailed"));
        }
        else {
            SponsorListText().Text(sponsorList);
        }
    }

    void HelpPage::SetupLocalization() {
        HelpAboutUid().Text(t(L"Help.Header.About"));
        HelpStarsDescUid().Text(t(L"Help.Text.Stars.Desc"));
        HelpBilibiliButtonUid().Content(tbox(L"Help.Button.Bilibili"));
        HelpGithubDescUid().Text(t(L"Help.Text.Github.Desc"));
        HelpGithubUserButtonUid().Content(tbox(L"Help.Button.GithubUser"));
        HelpGithubRepoButtonUid().Content(tbox(L"Help.Button.GithubRepo"));
        HelpAcknowledgementUid().Text(t(L"Help.Header.Acknowledgement"));
        HelpWinUIDescUid().Text(t(L"Help.Text.WinUI.Desc"));
        HelpWinUIButtonUid().Content(tbox(L"Help.Button.WinUI"));
        HelpWinUIEssentialsDescUid().Text(t(L"Help.Text.WinUIEssentials.Desc"));
        HelpKALIDescUid().Text(t(L"Help.Text.KALI.Desc"));
        HelpMuLinDescUid().Text(t(L"Help.Text.MuLin.Desc"));
        HelpWormwakerDescUid().Text(t(L"Help.Text.Wormwaker.Desc"));
        HelpSponsorsUid().Text(t(L"Help.Header.Sponsors"));
        HelpSponsorButtonUid().Content(tbox(L"Help.Button.Sponsor"));
        HelpSponsorIntroUid().Text(t(L"Help.Text.SponsorIntro"));
        SponsorListText().Text(t(L"Msg.Loading"));
    }
}




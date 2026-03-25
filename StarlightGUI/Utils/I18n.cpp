#include "pch.h"
#include "I18n.h"

#include <unordered_map>
#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace winrt::StarlightGUI::implementation {
    std::string NormalizeTag(std::string tag) {
        std::replace(tag.begin(), tag.end(), '_', '-');
        auto wide = winrt::StarlightGUI::implementation::StringToWideString(tag);
        wide = winrt::StarlightGUI::implementation::ToLowerCase(wide);
        return winrt::StarlightGUI::implementation::WideStringToString(wide);
    }

    std::vector<std::wstring> GetPreferredLanguages() {
        std::vector<std::wstring> langs;
        ULONG numLangs = 0;
        ULONG bufSize = 0;
        if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLangs, nullptr, &bufSize) || bufSize == 0) {
            return langs;
        }

        std::wstring buf(bufSize, L'\0');
        if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLangs, buf.data(), &bufSize)) {
            return langs;
        }

        const wchar_t* p = buf.c_str();
        while (*p) {
            langs.emplace_back(p);
            p += wcslen(p) + 1;
        }
        return langs;
    }

    std::vector<std::wstring> GetAvailableLanguages(std::wstring const& baseDir) {
        namespace fs = std::filesystem;
        std::vector<std::wstring> langs;
        fs::path stringsDir = fs::path(baseDir) / L"Strings";

        std::error_code ec;
        if (!fs::exists(stringsDir, ec) || ec) return langs;

        for (auto const& entry : fs::directory_iterator(stringsDir, ec)) {
            if (ec) break;
            if (!entry.is_directory()) continue;
            auto resPath = entry.path() / L"Resources.json";
            if (fs::exists(resPath, ec) && !ec) {
                langs.push_back(entry.path().filename().wstring());
            }
        }
        return langs;
    }

    std::wstring ResolveLanguageTag(std::wstring const& baseDir) {
        using namespace winrt::StarlightGUI::implementation;
        auto available = GetAvailableLanguages(baseDir);
        if (available.empty()) return L"zh-CN";

        std::unordered_map<std::string, std::wstring> normalizedToOriginal;
        for (auto const& lang : available) {
            std::string utf8(lang.begin(), lang.end());
            normalizedToOriginal[NormalizeTag(utf8)] = lang;
        }

        std::vector<std::wstring> candidates;
        if (language.empty() || _stricmp(language.c_str(), "system") == 0) {
            candidates = GetPreferredLanguages();
        }
        else {
            candidates.push_back(std::wstring(language.begin(), language.end()));
        }

        for (auto const& cand : candidates) {
            std::string u8(cand.begin(), cand.end());
            auto norm = NormalizeTag(u8);
            auto it = normalizedToOriginal.find(norm);
            if (it != normalizedToOriginal.end()) return it->second;
        }

        for (auto const& cand : candidates) {
            std::string u8(cand.begin(), cand.end());
            auto norm = NormalizeTag(u8);
            auto pos = norm.find('-');
            auto base = (pos == std::string::npos) ? norm : norm.substr(0, pos);
            for (auto const& kv : normalizedToOriginal) {
                auto p = kv.first.find('-');
                auto b = (p == std::string::npos) ? kv.first : kv.first.substr(0, p);
                if (b == base) return kv.second;
            }
        }

        auto en = normalizedToOriginal.find("en-us");
        if (en != normalizedToOriginal.end()) return en->second;
        auto zh = normalizedToOriginal.find("zh-cn");
        if (zh != normalizedToOriginal.end()) return zh->second;
        return available.front();
    }

    std::unordered_map<std::wstring, std::wstring> ParseJsonFile(const std::wstring& path) {
        std::unordered_map<std::wstring, std::wstring> map;

        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return map;
        std::string utf8((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (utf8.empty()) return map;

        if (utf8.size() >= 3 && utf8[0] == '\xEF' && utf8[1] == '\xBB' && utf8[2] == '\xBF') {
            utf8 = utf8.substr(3);
        }

        try {
            auto j = nlohmann::json::parse(utf8);
            if (!j.is_object()) return map;

            for (auto it = j.begin(); it != j.end(); ++it) {
                if (!it.value().is_string()) continue;
                auto key = winrt::StarlightGUI::implementation::StringToWideString(it.key());
                auto val = winrt::StarlightGUI::implementation::StringToWideString(it.value().get<std::string>());
                if (!key.empty()) map[key] = val;
            }
        }
        catch (...) {
        }
        return map;
    }

    const std::unordered_map<std::wstring, std::wstring>& GetStrings() {
        static std::unordered_map<std::wstring, std::wstring> s_map = [] {
            wchar_t exe[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe, MAX_PATH);
            std::wstring dir(exe);
            dir = dir.substr(0, dir.rfind(L'\\') + 1);

            std::wstring lang = ResolveLanguageTag(dir);
            auto map = ParseJsonFile(dir + L"Strings\\" + lang + L"\\Resources.json");
            if (map.empty()) {
                map = ParseJsonFile(dir + L"Strings\\zh-CN\\Resources.json");
                if (map.empty()) map = ParseJsonFile(dir + L"Strings\\en-US\\Resources.json");
            }
            return map;
        }();
        return s_map;
    }

    winrt::hstring GetLocalizedString(const wchar_t* key) noexcept {
        try {
            auto const& map = GetStrings();
            auto it = map.find(key);
            if (it != map.end()) {
                return winrt::hstring{ it->second };
            }
        }
        catch (...) {
        }
        return winrt::hstring{ key };
    }

    winrt::hstring t(const wchar_t* key) noexcept {
        return GetLocalizedString(key);
    }

    winrt::hstring t(std::wstring_view key) noexcept {
        return GetLocalizedString(std::wstring(key).c_str());
    }

    winrt::hstring t(const char* key) noexcept {
        return GetLocalizedString(StringToWideString(key ? std::string(key) : std::string()).c_str());
    }

    winrt::hstring t(std::string_view key) noexcept {
        return GetLocalizedString(StringToWideString(std::string(key)).c_str());
    }

    winrt::Windows::Foundation::IInspectable GetLocalizedInspectable(const wchar_t* key) noexcept {
        return winrt::box_value(GetLocalizedString(key));
    }

    winrt::Windows::Foundation::IInspectable tbox(const wchar_t* key) noexcept {
        return GetLocalizedInspectable(key);
    }

    winrt::Windows::Foundation::IInspectable tbox(std::wstring_view key) noexcept {
        return winrt::box_value(t(key));
    }

    winrt::Windows::Foundation::IInspectable tbox(const char* key) noexcept {
        return winrt::box_value(t(key));
    }

    winrt::Windows::Foundation::IInspectable tbox(std::string_view key) noexcept {
        return winrt::box_value(t(key));
    }
}

#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

extern int enumFileMode, backgroundType, micaType, acrylicType, navigationStyle, imageStretch;
extern std::string backgroundImage, language, theme;
extern bool enumStrengthen, functionShowDeprecated, functionShowUnknown, functionUseDocumentName, pdhFirst, elevatedRun, dangerousConfirm, checkUpdate, taskAutoRefresh, trayBackgroundRun, autoStopDriver, autoStart, replaceTaskManager;
extern bool hypervisorMode;
extern int imageOpacity, disasmCount;

namespace winrt::StarlightGUI::implementation {
    void InitializeConfig();
    json LoadConfigSnapshot();
    void SaveConfigSnapshot(const json& config);
    void WriteConfigValue(const std::string& key, const json& value);

    template<typename T>
    void SaveConfig(const std::string& key, const T& value) {
        try {
            WriteConfigValue(key, value);
        }
        catch (...) {
        }
    }

    template<typename T>
    T ReadConfig(const std::string& key, const T& defaultValue) {
        try {
            json config = LoadConfigSnapshot();
            if (config.contains(key)) {
                try {
                    return config[key].get<T>();
                }
                catch (...) {
                }
            }

            WriteConfigValue(key, defaultValue);
        }
        catch (...) {
        }
        return defaultValue;
    }
}

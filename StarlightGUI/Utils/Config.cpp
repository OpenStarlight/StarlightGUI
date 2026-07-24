#include "pch.h"
#include "Config.h"
#include "CppUtils.h"
#include <filesystem>
#include <fstream>
#include <mutex>

namespace {
    std::mutex configMutex;

    std::filesystem::path GetConfigPath() {
        return std::filesystem::path(winrt::StarlightGUI::implementation::GetInstalledLocationPath()) / "StarlightGUI.json";
    }

    json LoadConfigFile() {
        std::ifstream stream(GetConfigPath());
        if (!stream) return json::object();

        try {
            json config;
            stream >> config;
            return config.is_object() ? config : json::object();
        }
        catch (...) {
            return json::object();
        }
    }

    void SaveConfigFile(const json& config) {
        std::ofstream stream(GetConfigPath(), std::ios::trunc);
        if (stream) stream << config.dump(4);
    }
}

namespace winrt::StarlightGUI::implementation {
    json LoadConfigSnapshot() {
        std::lock_guard lock(configMutex);
        try {
            return LoadConfigFile();
        }
        catch (...) {
            return json::object();
        }
    }

    void SaveConfigSnapshot(const json& config) {
        std::lock_guard lock(configMutex);
        try {
            SaveConfigFile(config);
        }
        catch (...) {
        }
    }

    void WriteConfigValue(const std::string& key, const json& value) {
        std::lock_guard lock(configMutex);
        try {
            json config = LoadConfigFile();
            config[key] = value;
            SaveConfigFile(config);
        }
        catch (...) {
        }
    }

    void InitializeConfig() {
        json config = LoadConfigSnapshot();
        bool changed = false;

        auto read = [&]<typename T>(const char* key, const T& defaultValue) {
            if (config.contains(key)) {
                try {
                    return config[key].get<T>();
                }
                catch (...) {
                }
            }

            config[key] = defaultValue;
            changed = true;
            return defaultValue;
        };

        enumFileMode = read("enum_file_mode", 0);
        enumStrengthen = read("enum_strengthen", false);
        functionShowDeprecated = read("function_show_deprecated", false);
        functionShowUnknown = read("function_show_unknown", true);
        functionUseDocumentName = read("function_use_document_name", false);
        pdhFirst = read("pdh_first", true);
        backgroundType = read("background_type", 0);
        micaType = read("mica_type", 1);
        acrylicType = read("acrylic_type", 0);
        elevatedRun = read("elevated_run", false);
        dangerousConfirm = read("dangerous_confirm", true);
        checkUpdate = read("check_update", true);
        taskAutoRefresh = read("task_auto_refresh", true);
        trayBackgroundRun = read("tray_background_run", false);
        autoStopDriver = read("auto_stop_driver", false);
        autoStart = read("auto_start", false);
        replaceTaskManager = read("replace_taskmgr", false);
        navigationStyle = read("navigation_style", 0);
        backgroundImage = read("background_image", std::string());
        imageOpacity = read("image_opacity", 20);
        imageStretch = read("image_stretch", 3);
        disasmCount = read("disasm_count", 16);
        language = read("language", std::string("system"));
        theme = read("theme", std::string("system"));

        if (changed) SaveConfigSnapshot(config);
    }
}

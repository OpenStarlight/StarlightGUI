#include "pch.h"
#include "Config.h"

namespace winrt::StarlightGUI::implementation {
    void InitializeConfig() {
        enum_file_mode = ReadConfig("enum_file_mode", 0);
        enum_strengthen = ReadConfig("enum_strengthen", false);
        function_show_deprecated = ReadConfig("function_show_deprecated", false);
        function_show_unknown = ReadConfig("function_show_unknown", true);
        function_use_document_name = ReadConfig("function_use_document_name", false);
        pdh_first = ReadConfig("pdh_first", true);
        background_type = ReadConfig("background_type", 0);
        mica_type = ReadConfig("mica_type", 1);
        acrylic_type = ReadConfig("acrylic_type", 0);
        elevated_run = ReadConfig("elevated_run", false);
        dangerous_confirm = ReadConfig("dangerous_confirm", true);
        check_update = ReadConfig("check_update", true);
        task_auto_refresh = ReadConfig("task_auto_refresh", true);
        tray_background_run = ReadConfig("tray_background_run", false);
        auto_stop_driver = ReadConfig("auto_stop_driver", false);
        auto_start = ReadConfig("auto_start", false);
        replace_taskmgr = ReadConfig("replace_taskmgr", false);
        navigation_style = ReadConfig("navigation_style", 0);
        background_image = ReadConfig("background_image", std::string(""));
        image_opacity = ReadConfig("image_opacity", 20);
        image_stretch = ReadConfig("image_stretch", 3);
        disasm_count = ReadConfig("disasm_count", 16);
        language = ReadConfig("language", std::string("system"));
        theme = ReadConfig("theme", std::string("system"));
    }
}

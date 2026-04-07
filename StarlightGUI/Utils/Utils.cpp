#include "pch.h"
#include "Utils.h"
#include <winrt/XamlToolkit.WinUI.Controls.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <unordered_map>
#include <shellapi.h>
#include <cstring>
#include <cctype>
#include <algorithm>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Composition::SystemBackdrops;

namespace slg {
    std::unordered_map<std::wstring, ImageSource>& GetShellIconCacheStore()
    {
        static std::unordered_map<std::wstring, ImageSource> cache;
        return cache;
    }

    coroutine::coroutine() = default;

    coroutine coroutine::promise_type::get_return_object() const noexcept { return {}; }

    void coroutine::promise_type::return_void() const noexcept {}

    std::suspend_never coroutine::promise_type::initial_suspend() const noexcept { return {}; }

    std::suspend_never coroutine::promise_type::final_suspend() const noexcept { return {}; }

    void coroutine::promise_type::unhandled_exception() const noexcept
    {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const hresult_error& e) {
            LOG_ERROR(L"App", L"===== Unhandled exception detected! =====");
            LOG_ERROR(L"App", L"Type: 'hresult_error'");
            LOG_ERROR(L"App", L"Code: %d", e.code().value);
            LOG_ERROR(L"App", L"Message: %s", e.message().c_str());
            LOG_ERROR(L"App", L"=========================================");
        }
        catch (const std::exception& e) {
            LOG_ERROR(L"App", L"===== Unhandled exception detected! =====");
            LOG_ERROR(L"App", L"Type: 'std::exception'");
            LOG_ERROR(L"App", L"Message: %hs", e.what());
            LOG_ERROR(L"App", L"=========================================");
        }
        catch (...) {
            LOG_ERROR(L"App", L"===== Unhandled exception detected! =====");
            LOG_ERROR(L"App", L"Type: OTHER/UNKNOWN");
            LOG_ERROR(L"App", L"This should not happen!");
            LOG_ERROR(L"App", L"=========================================");
        }
    }
    Styles GetStyles()
    {
        auto resources = Application::Current().Resources();
        return {
            unbox_value<Style>(resources.TryLookup(box_value(L"MenuFlyoutItemStyle"))),
            unbox_value<Style>(resources.TryLookup(box_value(L"MenuFlyoutSubItemStyle")))
        };
    }

    MenuFlyoutItem CreateMenuItem(
        Styles const& styles,
        hstring const& glyph,
        hstring const& text,
        RoutedEventHandler const& click)
    {
        MenuFlyoutItem item;
        item.Style(styles.Item);
        item.Icon(CreateFontIcon(glyph));
        item.Text(text);
        if (click) item.Click(click);
        return item;
    }

    MenuFlyoutItem CreateMenuItem(
        Styles const& styles,
        hstring const& text,
        RoutedEventHandler const& click)
    {
        MenuFlyoutItem item;
        item.Style(styles.Item);
        item.Text(text);
        if (click) item.Click(click);
        return item;
    }

    MenuFlyoutSubItem CreateMenuSubItem(
        Styles const& styles,
        hstring const& glyph,
        hstring const& text)
    {
        MenuFlyoutSubItem item;
        item.Style(styles.SubItem);
        item.Icon(CreateFontIcon(glyph));
        item.Text(text);
        return item;
    }

    MenuFlyoutSubItem CreateMenuSubItem(
        Styles const& styles,
        hstring const& text)
    {
        MenuFlyoutSubItem item;
        item.Style(styles.SubItem);
        item.Text(text);
        return item;
    }

    void ShowAt(
        MenuFlyout const& flyout,
        ListView const& listView,
        RightTappedRoutedEventArgs const& e)
    {
        flyout.ShowAt(listView, e.GetPosition(listView));
    }
    FontIcon CreateFontIcon(hstring glyph) {
        FontIcon fontIcon;
        fontIcon.Glyph(glyph);
        fontIcon.FontFamily(FontFamily(L"Segoe Fluent Icons"));
        fontIcon.FontSize(16);

        return fontIcon;
    }

    InfoBar CreateInfoBar(hstring title, hstring message, InfoBarSeverity severity, XamlRoot xamlRoot) {
        InfoBar infobar;

        infobar.Title(title);
        infobar.Message(message);
        infobar.Severity(severity);
        infobar.XamlRoot(xamlRoot);
        infobar.RequestedTheme(GetConfiguredElementTheme());
        infobar.HorizontalAlignment(HorizontalAlignment::Right);
        infobar.VerticalAlignment(VerticalAlignment::Top);

        if (severity == InfoBarSeverity::Informational) {
            infobar.Background(SolidColorBrush{ slg::GetConfiguredElementTheme() == ElementTheme::Dark ? Color{ 255,40,40,40 } : Color{ 255,240,240,240 } });
        }

        return infobar;
    }

    void DisplayInfoBar(InfoBar infobar, Panel parent, int time) {
        if (!infobar || !parent) return;

        // Entrance animation
        EdgeUIThemeTransition transition;
        TransitionCollection transitions;
        transitions.Append(transition);
        infobar.Transitions(transitions);

        // Add and display
        parent.Children().Append(infobar);
        infobar.IsOpen(true);

        // Auto close timer
        auto timer = DispatcherTimer();
        timer.Interval(std::chrono::milliseconds(time));
        timer.Tick([infobar, parent, timer](auto&&, auto&&) {
            // Run fade out animation first
            Storyboard storyboard;
            auto fadeOutAnimation = FadeOutThemeAnimation();
            Storyboard::SetTarget(fadeOutAnimation, infobar);
            storyboard.Children().Append(fadeOutAnimation.as<Timeline>());
            storyboard.Begin();

            // Then close and remove from parent
            auto timer2 = DispatcherTimer();
            timer2.Interval(std::chrono::milliseconds(300));
            timer2.Tick([infobar, parent, timer2](auto&&, auto&&) {
                infobar.IsOpen(false);
                uint32_t index;
                if (parent.Children().IndexOf(infobar, index)) {
                    parent.Children().RemoveAt(index);
                }
                timer2.Stop();
                });
            timer2.Start();

            timer.Stop();
            });
        timer.Start();
    }

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, XamlRoot xamlRoot, Panel parent, int time) {
        DisplayInfoBar(CreateInfoBar(title, message, severity, xamlRoot), parent, time);
    }

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, StarlightGUI::implementation::MainWindow* instance, int time) {
        DisplayInfoBar(CreateInfoBar(title, message, severity, instance->MainWindowGrid().XamlRoot()), instance->InfoBarPanel(), time);
    }

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, StarlightGUI::implementation::InfoWindow* instance, int time) {
        DisplayInfoBar(CreateInfoBar(title, message, severity, instance->InfoWindowGrid().XamlRoot()), instance->InfoBarPanel(), time);
    }

    ContentDialog CreateContentDialog(hstring title, hstring content, hstring closeMessage, XamlRoot xamlRoot) {
        ContentDialog dialog;

        dialog.Title(box_value(title));
        dialog.Content(box_value(content));
        dialog.CloseButtonText(closeMessage);
        dialog.XamlRoot(xamlRoot);
        dialog.RequestedTheme(GetConfiguredElementTheme());

        return dialog;
    }

    DataTemplate GetContentDialogSuccessTemplate() {
        return XamlReader::Load(LR"(
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <StackPanel Orientation="Horizontal" Spacing="8">
                <FontIcon Glyph="&#xec61;" FontSize="30" FontFamily="Segoe Fluent Icons" Foreground="Green" Margin="0,5,0,0"/>
                <TextBlock Text="{Binding}" VerticalAlignment="Center"/>
            </StackPanel>
        </DataTemplate>
    )").as<DataTemplate>();
    }

    DataTemplate GetContentDialogErrorTemplate() {
        return XamlReader::Load(LR"(
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <StackPanel Orientation="Horizontal" Spacing="8">
                <FontIcon Glyph="&#xeb90;" FontSize="30" FontFamily="Segoe Fluent Icons" Foreground="OrangeRed" Margin="0,5,0,0"/>
                <TextBlock Text="{Binding}" VerticalAlignment="Center"/>
            </StackPanel>
        </DataTemplate>
    )").as<DataTemplate>();
    }

    DataTemplate GetContentDialogInfoTemplate() {
        return XamlReader::Load(LR"(
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <StackPanel Orientation="Horizontal" Spacing="8">
                <FontIcon Glyph="&#xf167;" FontSize="30" FontFamily="Segoe Fluent Icons" Foreground="LightBlue" Margin="0,5,0,0"/>
                <TextBlock Text="{Binding}" VerticalAlignment="Center"/>
            </StackPanel>
        </DataTemplate>
    )").as<DataTemplate>();
    }

    DataTemplate GetTemplate(hstring xaml) {
        return XamlReader::Load(xaml).as<DataTemplate>();
    }

    bool CheckIllegalComboBoxAction(IInspectable const& sender, SelectionChangedEventArgs const& e) {
        auto cb = sender.as<ComboBox>();

        if (!cb) return true;

        int index = cb.SelectedIndex();
        int itemCount = cb.Items().Size();

        // 非法索引，返回true并重置索引
        if (index < 0 || index >= itemCount) {
            cb.SelectedIndex(0);
            return true; 
        }

        // 正常索引，返回false
        return false;
    }

    static ElementTheme GetSystemElementTheme()
    {
        DWORD lightTheme = 1;
        DWORD size = sizeof(lightTheme);
        auto result = RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme",
            RRF_RT_REG_DWORD,
            nullptr,
            &lightTheme,
            &size);

        if (result == ERROR_SUCCESS) {
            return lightTheme == 0 ? ElementTheme::Dark : ElementTheme::Light;
        }

        return ElementTheme::Dark;
    }

    ElementTheme GetConfiguredElementTheme()
    {
        std::string themeValue = theme;
        std::transform(themeValue.begin(), themeValue.end(), themeValue.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (themeValue == "light") {
            return ElementTheme::Light;
        }

        if (themeValue == "dark") {
            return ElementTheme::Dark;
        }

        return GetSystemElementTheme();
    }

    void ApplyConfiguredTheme()
    {
        auto targetTheme = GetConfiguredElementTheme();
		BOOL isDark = (targetTheme == ElementTheme::Dark) ? TRUE : FALSE;

        if (g_mainWindowInstance) {
            DwmSetWindowAttribute(g_mainWindowInstance->GetWindowHandle(), DWMWA_USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));
            g_mainWindowInstance->MainWindowGrid().RequestedTheme(targetTheme);
            g_mainWindowInstance->LoadBackdrop();
        }

        if (g_infoWindowInstance) {
            DwmSetWindowAttribute(g_infoWindowInstance->GetWindowHandle(), DWMWA_USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));
            g_infoWindowInstance->InfoWindowGrid().RequestedTheme(targetTheme);
            g_infoWindowInstance->LoadBackdrop();
        }
    }

    ImageSource CreateImageSourceFromHIcon(HICON hIcon, int iconSize, bool destroyIcon)
    {
        if (!hIcon || iconSize <= 0) return nullptr;

        HDC screenDc = GetDC(nullptr);
        if (!screenDc) {
            if (destroyIcon) DestroyIcon(hIcon);
            return nullptr;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = iconSize;
        bmi.bmiHeader.biHeight = -iconSize;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP hBitmap = CreateDIBSection(screenDc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!hBitmap || !bits) {
            ReleaseDC(nullptr, screenDc);
            if (destroyIcon) DestroyIcon(hIcon);
            return nullptr;
        }

        HDC memDc = CreateCompatibleDC(screenDc);
        if (!memDc) {
            DeleteObject(hBitmap);
            ReleaseDC(nullptr, screenDc);
            if (destroyIcon) DestroyIcon(hIcon);
            return nullptr;
        }

        auto oldBitmap = SelectObject(memDc, hBitmap);
        std::memset(bits, 0, iconSize * iconSize * 4);
        DrawIconEx(memDc, 0, 0, hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

        Imaging::WriteableBitmap bitmap(iconSize, iconSize);
        std::memcpy(bitmap.PixelBuffer().data(), bits, iconSize * iconSize * 4);

        SelectObject(memDc, oldBitmap);
        DeleteDC(memDc);
        DeleteObject(hBitmap);
        ReleaseDC(nullptr, screenDc);

        if (destroyIcon) DestroyIcon(hIcon);
        return bitmap.as<ImageSource>();
    }

    ImageSource GetShellIconImage(
        std::wstring const& path,
        bool isDirectory,
        int iconSize,
        bool useFileAttributes,
        std::wstring const& cacheKey)
    {
        auto& cache = GetShellIconCacheStore();

        std::wstring key = cacheKey;
        if (key.empty()) key = (isDirectory ? L"dir:" : L"file:") + path;

        auto cacheIt = cache.find(key);
        if (cacheIt != cache.end()) return cacheIt->second;

        SHFILEINFO shfi{};
        UINT flags = SHGFI_ICON | SHGFI_SMALLICON;
        DWORD attrs = isDirectory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

        bool status = false;
        if (useFileAttributes) {
            status = SHGetFileInfoW(path.c_str(), attrs, &shfi, sizeof(shfi), flags | SHGFI_USEFILEATTRIBUTES) != 0;
            if (!status) status = SHGetFileInfoW(L".", FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), flags | SHGFI_USEFILEATTRIBUTES) != 0;
        }
        else {
            status = SHGetFileInfoW(path.c_str(), 0, &shfi, sizeof(shfi), flags) != 0;
            if (!status) status = SHGetFileInfoW(path.c_str(), attrs, &shfi, sizeof(shfi), flags | SHGFI_USEFILEATTRIBUTES) != 0;
            if (!status) status = SHGetFileInfoW(L".", FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), flags | SHGFI_USEFILEATTRIBUTES) != 0;
        }

        if (!status || !shfi.hIcon) return nullptr;

        auto source = CreateImageSourceFromHIcon(shfi.hIcon, iconSize, true);
        if (source) cache.insert_or_assign(key, source);
        return source;
    }

    void ClearShellIconCache()
    {
        GetShellIconCacheStore().clear();
    }

    void ApplyHeaderColumnWidthsToRow(
        Grid const& headerGrid,
        Grid const& rowGrid,
        uint32_t rowOffset)
    {
        if (!headerGrid || !rowGrid) return;

        auto headerColumns = headerGrid.ColumnDefinitions();
        auto rowColumns = rowGrid.ColumnDefinitions();
        if (headerColumns.Size() == 0 || rowColumns.Size() < rowOffset + headerColumns.Size()) return;

        for (uint32_t i = 0; i < headerColumns.Size(); ++i) {
            rowColumns.GetAt(rowOffset + i).Width(headerColumns.GetAt(i).Width());
        }
    }

    void ApplyHeaderColumnWidthsToContainer(
        Grid const& headerGrid,
        ListViewItem const& itemContainer,
        uint32_t rowOffset)
    {
        if (!headerGrid || !itemContainer) return;

        auto rowGrid = itemContainer.ContentTemplateRoot().try_as<Grid>();
        if (!rowGrid) return;

        ApplyHeaderColumnWidthsToRow(headerGrid, rowGrid, rowOffset);
    }

    void SyncListViewColumnWidths(
        Grid const& headerGrid,
        Grid const& bodyGrid,
        ListView const& listView,
        uint32_t rowOffset,
        double epsilon)
    {
        if (!headerGrid || !bodyGrid || !listView) return;

        auto headerColumns = headerGrid.ColumnDefinitions();
        auto bodyColumns = bodyGrid.ColumnDefinitions();
        if (headerColumns.Size() == 0 || bodyColumns.Size() < headerColumns.Size()) return;

        static std::unordered_map<uint64_t, std::vector<double>> cachedHeaderWidths;
        uint64_t key = reinterpret_cast<uint64_t>(get_abi(headerGrid));
        auto& lastWidths = cachedHeaderWidths[key];
        if (lastWidths.size() != headerColumns.Size()) {
            lastWidths.assign(headerColumns.Size(), -1.0);
        }

        bool changed = false;
        for (uint32_t i = 0; i < headerColumns.Size(); ++i) {
            double current = headerColumns.GetAt(i).ActualWidth();
            double previous = lastWidths[i];
            if (current > previous + epsilon || current + epsilon < previous) {
                changed = true;
                break;
            }
        }

        if (!changed) return;

        for (uint32_t i = 0; i < headerColumns.Size(); ++i) {
            auto headerColumn = headerColumns.GetAt(i);
            bodyColumns.GetAt(i).Width(headerColumn.Width());
            lastWidths[i] = headerColumn.ActualWidth();
        }

        auto itemCount = listView.Items().Size();
        for (uint32_t i = 0; i < itemCount; ++i) {
            auto itemContainer = listView.ContainerFromIndex(i).try_as<ListViewItem>();
            if (!itemContainer) continue;
            ApplyHeaderColumnWidthsToContainer(headerGrid, itemContainer, rowOffset);
        }
    }
}

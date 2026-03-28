#include "pch.h"
#include "Utils.h"
#include <winrt/XamlToolkit.WinUI.Controls.h>
#include <unordered_map>
#include <unordered_set>
#include <shellapi.h>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <limits>
#include <cmath>

namespace slg {
    static void UpdateTextMarqueeByNamesCore(
        winrt::Microsoft::UI::Xaml::FrameworkElement const& itemRoot,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding,
        int retryCount);

    static void UpdateVisibleListViewMarqueeByNamesCore(
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        uint32_t itemCount,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding)
    {
        if (!listView) return;
        auto liveCount = listView.Items().Size();
        if (liveCount > itemCount) itemCount = liveCount;

        auto panel = listView.ItemsPanelRoot().try_as<winrt::Microsoft::UI::Xaml::Controls::Panel>();
        if (panel) {
            auto children = panel.Children();
            for (uint32_t i = 0; i < children.Size(); ++i) {
                auto itemContainer = children.GetAt(i).try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
                if (!itemContainer) continue;

                auto contentRoot = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
                if (!contentRoot) continue;

                UpdateTextMarqueeByNames(contentRoot, containerName, textBlockName, marqueeName, widthPadding);
            }
            return;
        }

        for (uint32_t i = 0; i < itemCount; ++i) {
            auto itemContainer = listView.ContainerFromIndex(i).try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
            if (!itemContainer) continue;

            auto contentRoot = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
            if (!contentRoot) continue;

            UpdateTextMarqueeByNames(contentRoot, containerName, textBlockName, marqueeName, widthPadding);
        }
    }

    static void EnsureListViewMarqueeScrollHook(
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding,
        int retryCount = 0)
    {
        if (!listView) return;

        static std::unordered_set<uint64_t> hookedListViews;
        uint64_t key = reinterpret_cast<uint64_t>(winrt::get_abi(listView));
        if (hookedListViews.find(key) != hookedListViews.end()) return;

        auto scrollViewer = slg::FindVisualChild<winrt::Microsoft::UI::Xaml::Controls::ScrollViewer>(listView);
        if (!scrollViewer) {
            if (retryCount < 4) {
                listView.DispatcherQueue().TryEnqueue([listView, containerName, textBlockName, marqueeName, widthPadding, retryCount]() {
                    EnsureListViewMarqueeScrollHook(listView, containerName, textBlockName, marqueeName, widthPadding, retryCount + 1);
                    });
            }
            return;
        }

        hookedListViews.insert(key);
        scrollViewer.ViewChanged([listView, containerName, textBlockName, marqueeName, widthPadding](auto&&, auto&&) {
            if (!listView) return;
            auto count = listView.Items().Size();
            UpdateVisibleListViewMarqueeByNamesCore(listView, count, containerName, textBlockName, marqueeName, widthPadding);
            });
    }

    static void EnsureListViewMarqueeContainerHook(
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding)
    {
        if (!listView) return;

        static std::unordered_set<uint64_t> hookedListViews;
        uint64_t key = reinterpret_cast<uint64_t>(winrt::get_abi(listView));
        if (hookedListViews.find(key) != hookedListViews.end()) return;
        hookedListViews.insert(key);

        listView.ContainerContentChanging([listView, containerName, textBlockName, marqueeName, widthPadding](auto&&, auto&& args) {
            auto itemContainer = args.ItemContainer().try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
            if (!itemContainer) return;

            auto contentRoot = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
            if (!contentRoot) return;

            UpdateTextMarqueeByNames(contentRoot, containerName, textBlockName, marqueeName, widthPadding);

            // Virtualized rows may finish arrange in a later tick.
            if (listView) {
                listView.DispatcherQueue().TryEnqueue([contentRoot, containerName, textBlockName, marqueeName, widthPadding]() {
                    UpdateTextMarqueeByNames(contentRoot, containerName, textBlockName, marqueeName, widthPadding);
                    });
            }
            });
    }

    std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Media::ImageSource>& GetShellIconCacheStore()
    {
        static std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Media::ImageSource> cache;
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
        catch (const winrt::hresult_error& e) {
            LOG_ERROR(L"App", L"===== Unhandled exception detected! =====");
            LOG_ERROR(L"App", L"Type: 'winrt::hresult_error'");
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
        auto resources = winrt::Microsoft::UI::Xaml::Application::Current().Resources();
        return {
            winrt::unbox_value<winrt::Microsoft::UI::Xaml::Style>(resources.TryLookup(winrt::box_value(L"MenuFlyoutItemStyle"))),
            winrt::unbox_value<winrt::Microsoft::UI::Xaml::Style>(resources.TryLookup(winrt::box_value(L"MenuFlyoutSubItemStyle")))
        };
    }

    MenuFlyoutItem CreateMenuItem(
        Styles const& styles,
        hstring const& glyph,
        hstring const& text,
        winrt::Microsoft::UI::Xaml::RoutedEventHandler const& click)
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
        winrt::Microsoft::UI::Xaml::RoutedEventHandler const& click)
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
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyout const& flyout,
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
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

        auto colorObj = Application::Current().Resources().TryLookup(box_value(L"SystemChromeMediumColor"));
        if (colorObj) {
            auto color = unbox_value<Color>(colorObj);
            SolidColorBrush bg;
            bg.Color(color);
            bg.Opacity(0.9);
            infobar.Background(bg);
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

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, winrt::StarlightGUI::implementation::MainWindow* instance, int time) {
        DisplayInfoBar(CreateInfoBar(title, message, severity, instance->MainWindowGrid().XamlRoot()), instance->InfoBarPanel(), time);
    }

    void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, winrt::StarlightGUI::implementation::InfoWindow* instance, int time) {
        DisplayInfoBar(CreateInfoBar(title, message, severity, instance->InfoWindowGrid().XamlRoot()), instance->InfoBarPanel(), time);
    }

    ContentDialog CreateContentDialog(hstring title, hstring content, hstring closeMessage, XamlRoot xamlRoot) {
        ContentDialog dialog;

        dialog.Title(winrt::box_value(title));
        dialog.Content(winrt::box_value(content));
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

    static winrt::Microsoft::UI::Xaml::ElementTheme GetSystemElementTheme()
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
            return lightTheme == 0 ? winrt::Microsoft::UI::Xaml::ElementTheme::Dark : winrt::Microsoft::UI::Xaml::ElementTheme::Light;
        }

        return winrt::Microsoft::UI::Xaml::ElementTheme::Dark;
    }

    winrt::Microsoft::UI::Xaml::ElementTheme GetConfiguredElementTheme()
    {
        std::string themeValue = theme;
        std::transform(themeValue.begin(), themeValue.end(), themeValue.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (themeValue == "light") {
            return winrt::Microsoft::UI::Xaml::ElementTheme::Light;
        }

        if (themeValue == "dark") {
            return winrt::Microsoft::UI::Xaml::ElementTheme::Dark;
        }

        return GetSystemElementTheme();
    }

    void ApplyConfiguredTheme()
    {
        auto targetTheme = GetConfiguredElementTheme();

        if (winrt::StarlightGUI::implementation::g_mainWindowInstance) {
            winrt::StarlightGUI::implementation::g_mainWindowInstance->MainWindowGrid().RequestedTheme(targetTheme);
        }

        if (winrt::StarlightGUI::implementation::g_infoWindowInstance) {
            winrt::StarlightGUI::implementation::g_infoWindowInstance->InfoWindowGrid().RequestedTheme(targetTheme);
        }
    }

    winrt::Microsoft::UI::Xaml::Media::ImageSource CreateImageSourceFromHIcon(HICON hIcon, int iconSize, bool destroyIcon)
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

        winrt::Microsoft::UI::Xaml::Media::Imaging::WriteableBitmap bitmap(iconSize, iconSize);
        std::memcpy(bitmap.PixelBuffer().data(), bits, iconSize * iconSize * 4);

        SelectObject(memDc, oldBitmap);
        DeleteDC(memDc);
        DeleteObject(hBitmap);
        ReleaseDC(nullptr, screenDc);

        if (destroyIcon) DestroyIcon(hIcon);
        return bitmap.as<winrt::Microsoft::UI::Xaml::Media::ImageSource>();
    }

    winrt::Microsoft::UI::Xaml::Media::ImageSource GetShellIconImage(
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

    void UpdateTextMarqueeByNames(
        winrt::Microsoft::UI::Xaml::FrameworkElement const& itemRoot,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding)
    {
        UpdateTextMarqueeByNamesCore(itemRoot, containerName, textBlockName, marqueeName, widthPadding, 0);
    }

    static void UpdateTextMarqueeByNamesCore(
        winrt::Microsoft::UI::Xaml::FrameworkElement const& itemRoot,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding,
        int retryCount)
    {
        if (!itemRoot) return;

        auto textContainer = itemRoot.FindName(containerName).try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
        auto textBlock = itemRoot.FindName(textBlockName).try_as<winrt::Microsoft::UI::Xaml::Controls::TextBlock>();
        auto marquee = itemRoot.FindName(marqueeName).try_as<winrt::WinUI3Package::MarqueeText>();
        if (!textContainer || !textBlock || !marquee) return;

        double availableWidth = textContainer.ActualWidth();
        if (availableWidth <= 1.0) {
            if (retryCount < 3) {
                itemRoot.DispatcherQueue().TryEnqueue([itemRoot, containerName, textBlockName, marqueeName, widthPadding, retryCount]() {
                    UpdateTextMarqueeByNamesCore(itemRoot, containerName, textBlockName, marqueeName, widthPadding, retryCount + 1);
                    });
            }
            return;
        }

        bool shouldUseMarquee = false;
        if (!textBlock.Text().empty()) {
            winrt::Microsoft::UI::Xaml::Controls::TextBlock measureBlock;
            measureBlock.Text(textBlock.Text());
            measureBlock.FontFamily(textBlock.FontFamily());
            measureBlock.FontSize(textBlock.FontSize());
            measureBlock.FontWeight(textBlock.FontWeight());
            measureBlock.Measure({
                static_cast<float>(std::numeric_limits<float>::max()),
                static_cast<float>(std::numeric_limits<float>::max())
                });

            // Keep borderline overflow cases on marquee to avoid first-frame under-measure.
            shouldUseMarquee = std::ceil(measureBlock.DesiredSize().Width) >= std::floor(availableWidth - widthPadding);
        }

        textBlock.Visibility(shouldUseMarquee ? winrt::Microsoft::UI::Xaml::Visibility::Collapsed : winrt::Microsoft::UI::Xaml::Visibility::Visible);
        marquee.Visibility(shouldUseMarquee ? winrt::Microsoft::UI::Xaml::Visibility::Visible : winrt::Microsoft::UI::Xaml::Visibility::Collapsed);

        if (shouldUseMarquee) {
            auto targetText = textBlock.Text();
            if (marquee.Text() == targetText) {
                // Force text change to retrigger internal animation without calling control methods.
                marquee.Text(L"");
            }
            marquee.Text(targetText);

            // One extra tick for controls that miss first-frame animation startup.
            if (retryCount < 2) {
                itemRoot.DispatcherQueue().TryEnqueue([itemRoot, containerName, textBlockName, marqueeName, widthPadding, retryCount]() {
                    UpdateTextMarqueeByNamesCore(itemRoot, containerName, textBlockName, marqueeName, widthPadding, retryCount + 1);
                    });
            }
        }
        else {
            // Keep hidden marquee lightweight and avoid stale animation state.
            if (!marquee.Text().empty()) marquee.Text(L"");
        }
    }

    void UpdateVisibleListViewMarqueeByNames(
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        uint32_t itemCount,
        winrt::hstring const& containerName,
        winrt::hstring const& textBlockName,
        winrt::hstring const& marqueeName,
        double widthPadding)
    {
        if (!listView) return;

        EnsureListViewMarqueeContainerHook(listView, containerName, textBlockName, marqueeName, widthPadding);
        EnsureListViewMarqueeScrollHook(listView, containerName, textBlockName, marqueeName, widthPadding);
        UpdateVisibleListViewMarqueeByNamesCore(listView, itemCount, containerName, textBlockName, marqueeName, widthPadding);

        // Re-evaluate on next UI tick: first pass can happen before final arrange width is settled.
        listView.DispatcherQueue().TryEnqueue([listView, containerName, textBlockName, marqueeName, widthPadding]() {
            if (!listView) return;
            auto count = listView.Items().Size();
            UpdateVisibleListViewMarqueeByNamesCore(listView, count, containerName, textBlockName, marqueeName, widthPadding);
            });
    }

    void ApplyHeaderColumnWidthsToRow(
        winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid,
        winrt::Microsoft::UI::Xaml::Controls::Grid const& rowGrid,
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
        winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid,
        winrt::Microsoft::UI::Xaml::Controls::ListViewItem const& itemContainer,
        uint32_t rowOffset)
    {
        if (!headerGrid || !itemContainer) return;

        auto rowGrid = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::Controls::Grid>();
        if (!rowGrid) return;

        ApplyHeaderColumnWidthsToRow(headerGrid, rowGrid, rowOffset);
    }

    void SyncListViewColumnWidths(
        winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid,
        winrt::Microsoft::UI::Xaml::Controls::Grid const& bodyGrid,
        winrt::Microsoft::UI::Xaml::Controls::ListView const& listView,
        uint32_t rowOffset,
        double epsilon)
    {
        if (!headerGrid || !bodyGrid || !listView) return;

        auto headerColumns = headerGrid.ColumnDefinitions();
        auto bodyColumns = bodyGrid.ColumnDefinitions();
        if (headerColumns.Size() == 0 || bodyColumns.Size() < headerColumns.Size()) return;

        static std::unordered_map<uint64_t, std::vector<double>> cachedHeaderWidths;
        uint64_t key = reinterpret_cast<uint64_t>(winrt::get_abi(headerGrid));
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
            auto itemContainer = listView.ContainerFromIndex(i).try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
            if (!itemContainer) continue;
            ApplyHeaderColumnWidthsToContainer(headerGrid, itemContainer, rowOffset);
        }
    }
}

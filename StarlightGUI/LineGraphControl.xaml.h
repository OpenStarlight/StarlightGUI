#pragma once

#include "LineGraphControl.g.h"
#include <vector>
#include <map>
#include <string>
#include <limits>
#include <algorithm>
#include <winrt/Windows.UI.h>

namespace winrt::StarlightGUI::implementation
{
    struct DataSeries
    {
        std::wstring name;
        winrt::Windows::UI::Color color = winrt::Windows::UI::Colors::Red();
        std::vector<winrt::StarlightGUI::DataPoint> points;
        bool visible = true;
        double lineThickness = 2.0;

        DataSeries() = default;
        DataSeries(std::wstring_view name, winrt::Windows::UI::Color color)
            : name(name), color(color) {
        }
    };

    struct LineGraphControl : LineGraphControlT<LineGraphControl>
    {
        LineGraphControl();

        winrt::hstring Title() const { return m_title.c_str(); }
        void Title(winrt::hstring const& value);

        bool ShowGrid() const { return m_showGrid; }
        void ShowGrid(bool value);

        bool ShowLegend() const { return m_showLegend; }
        void ShowLegend(bool value);

        bool EnableZoom() const { return m_enableZoom; }
        void EnableZoom(bool value);

        void AddSeries(std::wstring_view name, winrt::Windows::UI::Color color);
        void AddDataPoint(std::wstring_view seriesName, double x, double y);
        void ClearSeries(std::wstring_view seriesName);
        void ClearAllSeries();

        void ZoomToFit();
        void ResetView();

        void SetSeriesColor(std::wstring_view seriesName, winrt::Windows::UI::Color color);
        void SetSeriesVisibility(std::wstring_view seriesName, bool visible);
        void SetSeriesThickness(std::wstring_view seriesName, double thickness);

        void OnSizeChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
        void OnPointerMoved(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void OnPointerExited(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void OnPointerWheelChanged(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

        void OnPointerMoved(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void OnPointerExited(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void OnPointerWheelChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

    private:
        void InitializeComponent();
        void InitializeChart();
        void UpdateChart();
        void CalculateBounds();

        double DataToScreenX(double dataX) const;
        double DataToScreenY(double dataY) const;
        winrt::StarlightGUI::DataPoint ScreenToData(const winrt::Windows::Foundation::Point& screenPoint) const;

        void DrawGrid();
        void DrawAxes();
        void DrawSeries();
        void DrawLegend();
        void UpdateCrosshair(const winrt::Windows::Foundation::Point& position);
        void UpdateTooltip(const winrt::Windows::Foundation::Point& position);
        void HandleZoom(const winrt::Windows::Foundation::Point& center, double delta);

        std::map<std::wstring, DataSeries> m_series;
        std::wstring m_title;
        bool m_showGrid = true;
        bool m_showLegend = true;
        bool m_enableZoom = true;

        double m_xMin = 0.0;
        double m_xMax = 1.0;
        double m_yMin = 0.0;
        double m_yMax = 1.0;
        double m_zoomLevel = 1.0;
        winrt::Windows::Foundation::Point m_viewOffset{ 0, 0 };

        std::vector<winrt::Windows::UI::Color> m_defaultColors = {
            winrt::Windows::UI::Colors::Red(),
            winrt::Windows::UI::Colors::Blue(),
            winrt::Windows::UI::Colors::Green(),
            winrt::Windows::UI::Colors::Orange(),
            winrt::Windows::UI::Colors::Purple(),
            winrt::Windows::UI::Colors::Cyan(),
            winrt::Windows::UI::Colors::Magenta(),
            winrt::Windows::UI::Colors::Gold()
        };

        static constexpr double DEFAULT_PADDING = 0.05;
        static constexpr double MIN_ZOOM = 0.1;
        static constexpr double MAX_ZOOM = 10.0;
        static constexpr double ZOOM_FACTOR = 0.1;
        static constexpr int MAX_POINTS = 10000;

        winrt::Microsoft::UI::Xaml::Controls::Canvas m_dataCanvas{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Canvas m_gridCanvas{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Canvas m_xAxisCanvas{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Canvas m_yAxisCanvas{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_titleText{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::StackPanel m_legendPanel{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Line m_crosshairX{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Line m_crosshairY{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Border m_tooltip{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_tooltipXText{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::StackPanel m_tooltipYPanel{ nullptr };
};
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct LineGraphControl : LineGraphControlT<LineGraphControl, implementation::LineGraphControl> {};
}

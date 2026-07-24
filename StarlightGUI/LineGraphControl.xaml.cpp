#include "pch.h"
#include "LineGraphControl.xaml.h"
#if __has_include("LineGraphControl.g.cpp")
#include "LineGraphControl.g.cpp"
#endif

#undef min
#undef max

#include <winrt/Microsoft.UI.Input.h>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Windows::UI;

namespace winrt::StarlightGUI::implementation
{
    LineGraphControl::LineGraphControl()
    {
        InitializeComponent();
        InitializeChart();
    }

    void LineGraphControl::InitializeComponent()
    {
        LineGraphControlT<LineGraphControl>::InitializeComponent();

        m_dataCanvas = DataCanvas();
        m_gridCanvas = GridCanvas();
        m_xAxisCanvas = XAxisCanvas();
        m_yAxisCanvas = YAxisCanvas();
        m_titleText = TitleText();
        m_legendPanel = LegendPanel();
        m_crosshairX = CrosshairX();
        m_crosshairY = CrosshairY();
        m_tooltip = Tooltip();
        m_tooltipXText = TooltipXText();
        m_tooltipYPanel = TooltipYPanel();
    }

    void LineGraphControl::InitializeChart()
    {
        m_xMin = 0.0;
        m_xMax = 1.0;
        m_yMin = 0.0;
        m_yMax = 1.0;

        UpdateChart();
    }

    void LineGraphControl::Title(winrt::hstring const& value)
    {
        m_title = value;
        m_titleText.Text(value);
        m_titleText.Visibility(value.empty() ? Visibility::Collapsed : Visibility::Visible);
    }

    void LineGraphControl::ShowGrid(bool value)
    {
        if (m_showGrid != value)
        {
            m_showGrid = value;
            UpdateChart();
        }
    }

    void LineGraphControl::ShowLegend(bool value)
    {
        if (m_showLegend != value)
        {
            m_showLegend = value;
            UpdateChart();
        }
    }

    void LineGraphControl::EnableZoom(bool value)
    {
        m_enableZoom = value;
    }

    void LineGraphControl::AddSeries(std::wstring_view name, winrt::Windows::UI::Color color)
    {
        std::wstring seriesName(name);

        if (m_series.find(seriesName) == m_series.end())
        {
            m_series[seriesName] = DataSeries(name, color);
            UpdateChart();
        }
    }

    void LineGraphControl::AddDataPoint(std::wstring_view seriesName, double x, double y)
    {
        std::wstring name(seriesName);
        auto it = m_series.find(name);

        if (it != m_series.end())
        {
            it->second.points.push_back(winrt::StarlightGUI::DataPoint{ x,y });

            // 超过最大数量就清空
            if (it->second.points.size() > MAX_POINTS)
            {
                it->second.points.erase(it->second.points.begin());
            }

            CalculateBounds();
            UpdateChart();
        }
    }

    void LineGraphControl::ClearSeries(std::wstring_view seriesName)
    {
        std::wstring name(seriesName);
        auto it = m_series.find(name);

        if (it != m_series.end())
        {
            it->second.points.clear();
            CalculateBounds();
            UpdateChart();
        }
    }

    void LineGraphControl::ClearAllSeries()
    {
        m_series.clear();
        InitializeChart();
    }

    void LineGraphControl::SetSeriesColor(std::wstring_view seriesName, winrt::Windows::UI::Color color)
    {
        std::wstring name(seriesName);
        auto it = m_series.find(name);

        if (it != m_series.end())
        {
            it->second.color = color;
            UpdateChart();
        }
    }

    void LineGraphControl::SetSeriesVisibility(std::wstring_view seriesName, bool visible)
    {
        std::wstring name(seriesName);
        auto it = m_series.find(name);

        if (it != m_series.end())
        {
            it->second.visible = visible;
            UpdateChart();
        }
    }

    void LineGraphControl::SetSeriesThickness(std::wstring_view seriesName, double thickness)
    {
        std::wstring name(seriesName);
        auto it = m_series.find(name);

        if (it != m_series.end())
        {
            it->second.lineThickness = thickness;
            UpdateChart();
        }
    }

    void LineGraphControl::ZoomToFit()
    {
        CalculateBounds();

        double xRange = m_xMax - m_xMin;
        double yRange = m_yMax - m_yMin;

        if (xRange == 0) xRange = 1.0;
        if (yRange == 0) yRange = 1.0;

        m_xMin -= xRange * DEFAULT_PADDING;
        m_xMax += xRange * DEFAULT_PADDING;
        m_yMin -= yRange * DEFAULT_PADDING;
        m_yMax += yRange * DEFAULT_PADDING;

        m_zoomLevel = 1.0;
        m_viewOffset = winrt::Windows::Foundation::Point(0, 0);

        UpdateChart();
    }

    void LineGraphControl::ResetView()
    {
        ZoomToFit();
    }

    void LineGraphControl::CalculateBounds()
    {
        if (m_series.empty())
        {
            m_xMin = 0.0;
            m_xMax = 1.0;
            m_yMin = 0.0;
            m_yMax = 1.0;
            return;
        }

        bool hasVisibleData = false;
        m_xMin = std::numeric_limits<double>::max();
        m_xMax = std::numeric_limits<double>::lowest();
        m_yMin = std::numeric_limits<double>::max();
        m_yMax = std::numeric_limits<double>::lowest();

        for (const auto& [name, series] : m_series)
        {
            if (!series.visible || series.points.empty())
                continue;

            hasVisibleData = true;

            for (const auto& point : series.points)
            {
                m_xMin = std::min(m_xMin, point.X);
                m_xMax = std::max(m_xMax, point.X);
                m_yMin = std::min(m_yMin, point.Y);
                m_yMax = std::max(m_yMax, point.Y);
            }
        }

        if (!hasVisibleData)
        {
            m_xMin = 0.0;
            m_xMax = 1.0;
            m_yMin = 0.0;
            m_yMax = 1.0;
        }
    }

    void LineGraphControl::UpdateChart()
    {
        if (ActualWidth() <= 0 || ActualHeight() <= 0)
            return;

        m_gridCanvas.Children().Clear();
        m_dataCanvas.Children().Clear();
        m_xAxisCanvas.Children().Clear();
        m_yAxisCanvas.Children().Clear();
        m_legendPanel.Children().Clear();

        if (m_showGrid) DrawGrid();
        DrawAxes();
        DrawSeries();
        if (m_showLegend) DrawLegend();
    }

    // 坐标转换
    double LineGraphControl::DataToScreenX(double dataX) const
    {
        double chartWidth = m_dataCanvas.ActualWidth();
        if (chartWidth <= 0) return 0;

        double dataWidth = m_xMax - m_xMin;
        if (dataWidth == 0) return chartWidth / 2;

        return (dataX - m_xMin) / dataWidth * chartWidth;
    }

    double LineGraphControl::DataToScreenY(double dataY) const
    {
        double chartHeight = m_dataCanvas.ActualHeight();
        if (chartHeight <= 0) return 0;

        double dataHeight = m_yMax - m_yMin;
        if (dataHeight == 0) return chartHeight / 2;

        return chartHeight - (dataY - m_yMin) / dataHeight * chartHeight;
    }

    winrt::StarlightGUI::DataPoint LineGraphControl::ScreenToData(const winrt::Windows::Foundation::Point& screenPoint) const
    {
        double chartWidth = m_dataCanvas.ActualWidth();
        double chartHeight = m_dataCanvas.ActualHeight();

        if (chartWidth <= 0 || chartHeight <= 0)
            return winrt::StarlightGUI::DataPoint{ 0.0, 0.0 };

        double dataWidth = m_xMax - m_xMin;
        double dataHeight = m_yMax - m_yMin;

        double dataX = m_xMin + (screenPoint.X / chartWidth) * dataWidth;
        double dataY = m_yMin + ((chartHeight - screenPoint.Y) / chartHeight) * dataHeight;

        return winrt::StarlightGUI::DataPoint{ dataX, dataY };
    }

    void LineGraphControl::DrawGrid()
    {
        double chartWidth = m_dataCanvas.ActualWidth();
        double chartHeight = m_dataCanvas.ActualHeight();

        if (chartWidth <= 0 || chartHeight <= 0)
            return;

        // 垂直网格线
        int verticalLines = 10;
        for (int i = 0; i <= verticalLines; i++)
        {
            double x = (chartWidth / verticalLines) * i;

            auto line = Line();
            line.X1(x);
            line.Y1(0);
            line.X2(x);
            line.Y2(chartHeight);
            line.Style(unbox_value<winrt::Microsoft::UI::Xaml::Style>(Resources().Lookup(winrt::box_value(L"GridLineStyle"))));

            m_gridCanvas.Children().Append(line);
        }

        // 水平网格线
        int horizontalLines = 10;
        for (int i = 0; i <= horizontalLines; i++)
        {
            double y = (chartHeight / horizontalLines) * i;

            auto line = Line();
            line.X1(0);
            line.Y1(y);
            line.X2(chartWidth);
            line.Y2(y);
            line.Style(unbox_value<winrt::Microsoft::UI::Xaml::Style>(Resources().Lookup(winrt::box_value(L"GridLineStyle"))));

            m_gridCanvas.Children().Append(line);
        }
    }

    void LineGraphControl::DrawAxes()
    {
        double chartWidth = m_dataCanvas.ActualWidth();
        double chartHeight = m_dataCanvas.ActualHeight();

        // X
        int xTicks = 6;
        for (int i = 0; i <= xTicks; i++)
        {
            double xValue = m_xMin + (m_xMax - m_xMin) * i / xTicks;
            double xPos = (chartWidth / xTicks) * i;

            // 刻度线
            auto tick = Line();
            tick.X1(xPos);
            tick.Y1(0);
            tick.X2(xPos);
            tick.Y2(5);
            tick.Stroke(SolidColorBrush(Colors::LightGray()));
            tick.StrokeThickness(1);
            m_xAxisCanvas.Children().Append(tick);

            // 标签
            auto label = TextBlock();
            std::wstringstream ss;
            ss << std::fixed << std::setprecision(2) << xValue;
            label.Text(ss.str());
            label.Style(unbox_value<Microsoft::UI::Xaml::Style>(Resources().Lookup(winrt::box_value(L"AxisLabelStyle"))));

            auto transform = TranslateTransform();
            transform.X(xPos - 10);
            transform.Y(10);
            label.RenderTransform(transform);

            m_xAxisCanvas.Children().Append(label);
        }

        // Y
        int yTicks = 6;
        for (int i = 0; i <= yTicks; i++)
        {
            double yValue = m_yMin + (m_yMax - m_yMin) * i / yTicks;
            double yPos = chartHeight - (chartHeight / yTicks) * i;

            // 刻度线
            auto tick = Line();
            tick.X1(m_yAxisCanvas.ActualWidth() - 5);
            tick.Y1(yPos);
            tick.X2(m_yAxisCanvas.ActualWidth());
            tick.Y2(yPos);
            tick.Stroke(SolidColorBrush(Colors::LightGray()));
            tick.StrokeThickness(1);
            m_yAxisCanvas.Children().Append(tick);

            // 标签
            auto label = TextBlock();
            std::wstringstream ss;
            ss << std::fixed << std::setprecision(2) << yValue;
            label.Text(ss.str());
            label.Style(unbox_value<Microsoft::UI::Xaml::Style>(Resources().Lookup(winrt::box_value(L"AxisLabelStyle"))));

            auto transform = TranslateTransform();
            transform.X(m_yAxisCanvas.ActualWidth() - 35);
            transform.Y(yPos - 8);
            label.RenderTransform(transform);

            m_yAxisCanvas.Children().Append(label);
        }
    }

    void LineGraphControl::DrawSeries()
    {
        if (m_series.empty())
            return;

        int colorIndex = 0;

        for (auto& [name, series] : m_series)
        {
            if (!series.visible || series.points.size() < 2)
                continue;

            if (series.color.A == 0)
            {
                series.color = m_defaultColors[colorIndex % m_defaultColors.size()];
                colorIndex++;
            }

            // 折线
            auto polyline = Microsoft::UI::Xaml::Shapes::Polyline();
            PointCollection points;

            for (const auto& point : series.points)
            {
                double screenX = DataToScreenX(point.X);
                double screenY = DataToScreenY(point.Y);
                points.Append(winrt::Windows::Foundation::Point(screenX, screenY));
            }

            polyline.Points(points);
            polyline.Stroke(SolidColorBrush(series.color));
            polyline.StrokeThickness(series.lineThickness);
            polyline.StrokeLineJoin(PenLineJoin::Round);

            m_dataCanvas.Children().Append(polyline);

            // 数据点标记
            if (series.points.size() <= 100)
            {
                for (const auto& point : series.points)
                {
                    double screenX = DataToScreenX(point.X);
                    double screenY = DataToScreenY(point.Y);

                    auto ellipse = Microsoft::UI::Xaml::Shapes::Ellipse();
                    ellipse.Width(6);
                    ellipse.Height(6);
                    ellipse.Fill(SolidColorBrush(series.color));

                    auto transform = TranslateTransform();
                    transform.X(screenX - 3);
                    transform.Y(screenY - 3);
                    ellipse.RenderTransform(transform);

                    m_dataCanvas.Children().Append(ellipse);
                }
            }
        }
    }

    void LineGraphControl::DrawLegend()
    {
        if (m_series.empty())
        {
            m_legendPanel.Visibility(Visibility::Collapsed);
            return;
        }

        int colorIndex = 0;

        for (const auto& [name, series] : m_series)
        {
            if (!series.visible)
                continue;

            auto legendItem = StackPanel();
            legendItem.Orientation(Orientation::Horizontal);
            legendItem.Margin(ThicknessHelper::FromLengths(8, 2, 0, 2));

            auto colorRect = Microsoft::UI::Xaml::Shapes::Rectangle();
            colorRect.Width(12);
            colorRect.Height(2);
            colorRect.Fill(SolidColorBrush(series.color));
            colorRect.Margin(ThicknessHelper::FromLengths(0, 0, 4, 0));

            auto nameText = TextBlock();
            nameText.Text(name);
            nameText.Foreground(SolidColorBrush(Colors::LightGray()));
            nameText.FontSize(12);
            nameText.VerticalAlignment(VerticalAlignment::Center);

            legendItem.Children().Append(colorRect);
            legendItem.Children().Append(nameText);

            m_legendPanel.Children().Append(legendItem);
        }

        if (m_legendPanel.Children().Size() > 0)
        {
            m_legendPanel.Visibility(Visibility::Visible);
        }
        else
        {
            m_legendPanel.Visibility(Visibility::Collapsed);
        }
    }

    void LineGraphControl::UpdateCrosshair(const winrt::Windows::Foundation::Point& position)
    {
        double chartWidth = m_dataCanvas.ActualWidth();
        double chartHeight = m_dataCanvas.ActualHeight();

        if (position.X < 0 || position.X > chartWidth ||
            position.Y < 0 || position.Y > chartHeight)
        {
            m_crosshairX.Visibility(Visibility::Collapsed);
            m_crosshairY.Visibility(Visibility::Collapsed);
            return;
        }

        m_crosshairX.X1(0);
        m_crosshairX.Y1(position.Y);
        m_crosshairX.X2(chartWidth);
        m_crosshairX.Y2(position.Y);
        m_crosshairX.Visibility(Visibility::Visible);

        m_crosshairY.X1(position.X);
        m_crosshairY.Y1(0);
        m_crosshairY.X2(position.X);
        m_crosshairY.Y2(chartHeight);
        m_crosshairY.Visibility(Visibility::Visible);
    }

    void LineGraphControl::UpdateTooltip(const winrt::Windows::Foundation::Point& position)
    {
        auto dataPoint = ScreenToData(position);

        std::wstringstream xss;
        xss << L"X: " << std::fixed << std::setprecision(2) << dataPoint.X;
        m_tooltipXText.Text(xss.str());

        m_tooltipYPanel.Children().Clear();

        // 查找最近的数据点
        for (const auto& [name, series] : m_series)
        {
            if (!series.visible || series.points.empty())
                continue;

            auto closest = std::min_element(series.points.begin(), series.points.end(),
                [&dataPoint](const winrt::StarlightGUI::DataPoint& a, const winrt::StarlightGUI::DataPoint& b) {
                    return std::abs(a.X - dataPoint.X) < std::abs(b.X - dataPoint.X);
                });

            if (closest != series.points.end())
            {
                double xRange = m_xMax - m_xMin;
                double yRange = m_yMax - m_yMin;
                double tolerance = xRange * 0.1 + yRange * 0.1; // 10% 范围

                if (std::abs(std::sqrt(std::pow(closest->X - dataPoint.X, 2) + std::pow(closest->Y - dataPoint.Y, 2))) <= tolerance)
                {
                    auto yItem = StackPanel();
                    yItem.Orientation(Orientation::Horizontal);

                    auto dot = Microsoft::UI::Xaml::Shapes::Ellipse();
                    dot.Width(6);
                    dot.Height(6);
                    dot.Fill(SolidColorBrush(series.color));
                    dot.Margin(ThicknessHelper::FromLengths(0, 0, 4, 0));

                    std::wstringstream yss;
                    yss << name << L": " << std::fixed << std::setprecision(2) << closest->Y;

                    auto yText = TextBlock();
                    yText.Text(yss.str());
                    yText.Foreground(SolidColorBrush(Colors::Black()));
                    yText.FontSize(12);
                    yText.VerticalAlignment(VerticalAlignment::Center);

                    yItem.Children().Append(dot);
                    yItem.Children().Append(yText);

                    m_tooltipYPanel.Children().Append(yItem);
                }
            }
        }

        if (m_tooltipYPanel.Children().Size() > 0)
        {
            m_tooltip.Visibility(Visibility::Visible);

            // 定位
            double tooltipWidth = m_tooltip.ActualWidth();
            double tooltipHeight = m_tooltip.ActualHeight();
            double left = position.X + 10;
            double top = position.Y - tooltipHeight / 2;

            if (left + tooltipWidth > m_dataCanvas.ActualWidth())
                left = position.X - tooltipWidth - 10;
            if (top < 0)
                top = 0;
            if (top + tooltipHeight > m_dataCanvas.ActualHeight())
                top = m_dataCanvas.ActualHeight() - tooltipHeight;

            auto transform = m_tooltip.RenderTransform().try_as<TranslateTransform>();
            if (!transform)
            {
                transform = TranslateTransform();
                m_tooltip.RenderTransform(transform);
            }

            transform.X(left);
            transform.Y(top);
        }
        else
        {
            m_tooltip.Visibility(Visibility::Collapsed);
        }
    }

    void LineGraphControl::HandleZoom(const winrt::Windows::Foundation::Point& center, double delta)
    {
        if (!m_enableZoom)
            return;

        double oldZoom = m_zoomLevel;
        m_zoomLevel = std::clamp(m_zoomLevel + delta * ZOOM_FACTOR, MIN_ZOOM, MAX_ZOOM);

        if (std::abs(oldZoom - m_zoomLevel) > 0.001)
        {
            auto dataCenter = ScreenToData(center);

            double zoomRatio = m_zoomLevel / oldZoom;
            double newXRange = (m_xMax - m_xMin) / zoomRatio;
            double newYRange = (m_yMax - m_yMin) / zoomRatio;

            m_xMin = dataCenter.X - (dataCenter.X - m_xMin) / zoomRatio;
            m_xMax = m_xMin + newXRange;

            m_yMin = dataCenter.Y - (dataCenter.Y - m_yMin) / zoomRatio;
            m_yMax = m_yMin + newYRange;

            UpdateChart();
        }
    }

    void LineGraphControl::OnSizeChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        UpdateChart();
    }

    void LineGraphControl::OnPointerMoved(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        auto point = e.GetCurrentPoint(*this);
        auto position = point.Position();

        // 减去边距偏移
        double xOffset = m_yAxisCanvas.ActualWidth() + 8;
        double yOffset = m_titleText.ActualHeight() + 8;

        winrt::Windows::Foundation::Point chartPosition(
            position.X - xOffset,
            position.Y - yOffset
        );

        if (chartPosition.X >= 0 && chartPosition.X <= m_dataCanvas.ActualWidth() &&
            chartPosition.Y >= 0 && chartPosition.Y <= m_dataCanvas.ActualHeight())
        {
            UpdateCrosshair(chartPosition);
            UpdateTooltip(chartPosition);
        }
        else
        {
            m_crosshairX.Visibility(Visibility::Collapsed);
            m_crosshairY.Visibility(Visibility::Collapsed);
            m_tooltip.Visibility(Visibility::Collapsed);
        }
    }

    void LineGraphControl::OnPointerExited(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        m_crosshairX.Visibility(Visibility::Collapsed);
        m_crosshairY.Visibility(Visibility::Collapsed);
        m_tooltip.Visibility(Visibility::Collapsed);
    }

    void LineGraphControl::OnPointerWheelChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        if (!m_enableZoom)
            return;

        auto point = e.GetCurrentPoint(*this);
        double delta = point.Properties().MouseWheelDelta();

        if (delta != 0)
        {
            // 计算在图表区域内的位置
            double xOffset = m_yAxisCanvas.ActualWidth() + 8;
            double yOffset = m_titleText.ActualHeight() + 8;

            winrt::Windows::Foundation::Point chartPosition(
                point.Position().X - xOffset,
                point.Position().Y - yOffset
            );

            HandleZoom(chartPosition, delta > 0 ? 1 : -1);
        }
    }

    void LineGraphControl::OnPointerMoved(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
    }

    void LineGraphControl::OnPointerExited(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
    }

    void LineGraphControl::OnPointerWheelChanged(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
    }
}

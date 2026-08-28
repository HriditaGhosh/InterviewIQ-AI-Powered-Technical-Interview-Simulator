#include "ChartManager.h"

#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAbstractAxis>
#include <QBrush>
#include <QPen>

namespace {

// Dark-theme palette shared with ThemeManager's dark stylesheet, so charts
// blend into the rest of the dark UI instead of showing a white plot area.
const QColor kChartBg("#1e1f22");
const QColor kChartPlotArea("#2b2d30");
const QColor kChartGrid("#3c3f41");
const QColor kChartText("#e6e6e6");

void applyDarkChartStyle(QChart *chart, QChartView *view)
{
    chart->setBackgroundBrush(QBrush(kChartBg));
    chart->setBackgroundPen(Qt::NoPen);
    chart->setPlotAreaBackgroundBrush(QBrush(kChartPlotArea));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setTitleBrush(QBrush(kChartText));

    if (chart->legend()) {
        chart->legend()->setLabelColor(kChartText);
        chart->legend()->setBackgroundVisible(false);
    }

    const auto axes = chart->axes();
    for (QAbstractAxis *axis : axes) {
        axis->setLabelsColor(kChartText);
        axis->setLinePenColor(kChartGrid);
        axis->setGridLineColor(kChartGrid);
        axis->setTitleBrush(QBrush(kChartText));
    }

    // The QGraphicsView viewport paints outside the chart's own background
    // brush, so match it too or a light border can peek through.
    view->setBackgroundBrush(QBrush(kChartBg));
    view->setStyleSheet("border: none;");
}

} // namespace

QChartView *ChartManager::buildScoreBreakdownPie(double technical, double communication, double confidence, QWidget *parent, bool darkTheme)
{
    auto *series = new QPieSeries();
    series->append("Technical", technical);
    series->append("Communication", communication);
    series->append("Confidence", confidence);
    for (auto *slice : series->slices()) {
        slice->setLabelVisible(true);
        if (darkTheme) {
            slice->setLabelBrush(QBrush(kChartText));
            slice->setBorderColor(kChartBg);
            slice->setBorderWidth(2);
        }
    }

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Score Breakdown");
    chart->legend()->setVisible(true);

    auto *view = new QChartView(chart, parent);
    view->setRenderHint(QPainter::Antialiasing);
    // Only style for dark mode when displayed on-screen next to the rest of
    // the dark UI. The PDF export path renders this onto a white page, so it
    // must keep the default light chart theme or the grabbed pixmap comes
    // out as a dark box embedded in an otherwise white report.
    if (darkTheme) {
        applyDarkChartStyle(chart, view);
    }
    return view;
}

QChartView *ChartManager::buildCategoryBarChart(const QVector<QPair<QString, double>> &categoryScores, QWidget *parent)
{
    auto *set = new QBarSet("Average Score");
    QStringList categories;
    for (const auto &pair : categoryScores) {
        categories << pair.first;
        *set << pair.second;
    }

    auto *series = new QBarSeries();
    series->append(set);

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Score by Category");

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    auto *view = new QChartView(chart, parent);
    view->setRenderHint(QPainter::Antialiasing);
    applyDarkChartStyle(chart, view);
    return view;
}

QChartView *ChartManager::buildProgressLineChart(const QVector<QPair<QString, double>> &dateVsScore, QWidget *parent)
{
    auto *series = new QLineSeries();
    int i = 0;
    QStringList labels;
    for (const auto &pair : dateVsScore) {
        series->append(i++, pair.second);
        labels << pair.first;
    }

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Progress Over Time");
    chart->createDefaultAxes();
    // TODO: swap the numeric X axis for a QBarCategoryAxis / QDateTimeAxis
    // using `labels` so dates are shown instead of indices.

    auto *view = new QChartView(chart, parent);
    view->setRenderHint(QPainter::Antialiasing);
    applyDarkChartStyle(chart, view);
    return view;
}

QChartView *ChartManager::buildEyeContactTimeline(const QVector<QPair<double, bool>> &timeVsLookingAtCamera, QWidget *parent)
{
    auto *series = new QLineSeries();
    for (const auto &sample : timeVsLookingAtCamera) {
        series->append(sample.first, sample.second ? 1.0 : 0.0);
    }

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Eye Contact Timeline");
    chart->createDefaultAxes();

    auto *view = new QChartView(chart, parent);
    view->setRenderHint(QPainter::Antialiasing);
    applyDarkChartStyle(chart, view);
    return view;
}

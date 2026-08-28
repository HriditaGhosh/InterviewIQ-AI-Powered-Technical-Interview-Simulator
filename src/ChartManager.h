#pragma once

#include <QWidget>
#include <QtCharts/QChartView>
#include <QVector>
#include <QPair>
#include <QString>

/**
 * Builds Qt Charts views for interview analytics:
 * pie chart (score breakdown), bar chart (category comparison),
 * line chart (progress over time / eye-contact timeline).
 */
class ChartManager {
public:
    static QChartView *buildScoreBreakdownPie(double technical, double communication, double confidence, QWidget *parent = nullptr, bool darkTheme = true);
    static QChartView *buildCategoryBarChart(const QVector<QPair<QString, double>> &categoryScores, QWidget *parent = nullptr);
    static QChartView *buildProgressLineChart(const QVector<QPair<QString, double>> &dateVsScore, QWidget *parent = nullptr);
    static QChartView *buildEyeContactTimeline(const QVector<QPair<double, bool>> &timeVsLookingAtCamera, QWidget *parent = nullptr);
};

#pragma once

#include <QString>
#include <QJsonObject>
#include <QPixmap>

/**
 * Generates the professional PDF interview report described in the spec:
 * candidate info, scores, technical/communication analysis, eye contact,
 * emotion, speaking speed, AI suggestions, final rating.
 *
 * Uses QPdfWriter + QPainter directly for full layout control.
 */
class PdfReportGenerator {
public:
    // `report` is expected to contain keys like: candidateName, category,
    // technicalScore, communicationScore, confidenceScore, eyeContactPct,
    // speakingWpm, aiSuggestions (array), overallRating.
    //
    // `scoreChart`, if non-null, is embedded under the Scores section —
    // typically a QPixmap grabbed from a ChartManager::buildScoreBreakdownPie()
    // QChartView via `chartView->grab()`.
    static bool generate(const QJsonObject &report, const QString &outputPath,
                          const QPixmap &scoreChart = QPixmap());
};

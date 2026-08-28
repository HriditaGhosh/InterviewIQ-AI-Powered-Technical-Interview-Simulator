#include "PdfReportGenerator.h"

#include <QPdfWriter>
#include <QPainter>
#include <QJsonArray>
#include <QFont>

bool PdfReportGenerator::generate(const QJsonObject &report, const QString &outputPath, const QPixmap &scoreChart)
{
    QPdfWriter writer(outputPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(150);

    QPainter painter(&writer);
    if (!painter.isActive()) return false;

    int y = 100;
    const int leftMargin = 100;
    const int lineHeight = 45;

    QFont titleFont("Arial", 20, QFont::Bold);
    QFont sectionFont("Arial", 14, QFont::Bold);
    QFont bodyFont("Arial", 11);

    painter.setFont(titleFont);
    painter.drawText(leftMargin, y, "InterviewIQ — Interview Report");
    y += lineHeight * 2;

    painter.setFont(sectionFont);
    painter.drawText(leftMargin, y, "Candidate Information");
    y += lineHeight;

    painter.setFont(bodyFont);
    painter.drawText(leftMargin, y, QString("Name: %1").arg(report.value("candidateName").toString("N/A")));
    y += lineHeight;
    painter.drawText(leftMargin, y, QString("Category: %1").arg(report.value("category").toString("N/A")));
    y += lineHeight * 2;

    painter.setFont(sectionFont);
    painter.drawText(leftMargin, y, "Scores");
    y += lineHeight;
    painter.setFont(bodyFont);
    painter.drawText(leftMargin, y, QString("Technical Score: %1").arg(report.value("technicalScore").toDouble()));
    y += lineHeight;
    painter.drawText(leftMargin, y, QString("Communication Score: %1").arg(report.value("communicationScore").toDouble()));
    y += lineHeight;
    painter.drawText(leftMargin, y, QString("Confidence Score: %1").arg(report.value("confidenceScore").toDouble()));
    y += lineHeight;
    painter.drawText(leftMargin, y, QString("Eye Contact: %1%").arg(report.value("eyeContactPct").toDouble()));
    y += lineHeight;
    painter.drawText(leftMargin, y, QString("Speaking Speed: %1 WPM").arg(report.value("speakingWpm").toDouble()));
    y += lineHeight;

    // Embed the score-breakdown chart (rendered elsewhere via
    // ChartManager::buildScoreBreakdownPie(...)->grab()) to the right of
    // the text, if one was provided.
    if (!scoreChart.isNull()) {
        const int chartWidth = 900;
        const int chartHeight = scoreChart.height() * chartWidth / qMax(1, scoreChart.width());
        painter.drawPixmap(leftMargin, y, chartWidth, chartHeight, scoreChart);
        y += chartHeight + lineHeight;
    } else {
        y += lineHeight;
    }

    painter.setFont(sectionFont);
    painter.drawText(leftMargin, y, "AI Suggestions");
    y += lineHeight;
    painter.setFont(bodyFont);
    for (const QJsonValue &v : report.value("aiSuggestions").toArray()) {
        painter.drawText(leftMargin, y, QString(u8"• %1").arg(v.toString()));
        y += lineHeight;
    }
    y += lineHeight;

    painter.setFont(sectionFont);
    painter.drawText(leftMargin, y, QString("Final Rating: %1").arg(report.value("overallRating").toString("N/A")));

    painter.end();
    return true;
}

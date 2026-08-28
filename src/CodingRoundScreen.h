#pragma once

#include <QWidget>
#include "CodingProblem.h"

class QComboBox;
class QLabel;
class QTextEdit;
class QPushButton;
class QPlainTextEdit;

/**
 * The Coding Interview Module (spec module 12): pick a problem, edit C++
 * in a built-in editor, run it against sample test cases, then submit to
 * run against sample + hidden test cases with pass/fail and timing per
 * case. Compilation/execution is handled by CodingJudge (needs g++ on
 * PATH — see CodingJudge.h).
 *
 * Note: memory usage per run is not measured (would need platform-specific
 * process APIs) — only pass/fail and execution time are reported.
 */
class CodingRoundScreen : public QWidget {
    Q_OBJECT
public:
    explicit CodingRoundScreen(QWidget *parent = nullptr);

signals:
    void backRequested();

private slots:
    void onDifficultyFilterChanged(int filterIndex);
    void onProblemChanged(int comboIndex);
    void onRunSampleClicked();
    void onSubmitClicked();

private:
    void populateProblemList();
    void runAndShowResults(bool includeHidden);
    QString formatTestCase(const CodingTestCase &test) const;

    QComboBox *m_difficultyFilterBox = nullptr;
    QComboBox *m_problemBox = nullptr;
    QLabel *m_descriptionLabel = nullptr;
    QTextEdit *m_codeEditor = nullptr;
    QPlainTextEdit *m_resultsView = nullptr;
    QPushButton *m_runSampleBtn = nullptr;
    QPushButton *m_submitBtn = nullptr;
    QLabel *m_statusLabel = nullptr;

    // m_problemBox's item at position i corresponds to
    // CodingProblemBank::all()[m_visibleIndices[i]] — needed because the
    // difficulty filter hides some problems, so combo-box position and
    // bank index aren't the same thing once a filter is active.
    QVector<int> m_visibleIndices;
};

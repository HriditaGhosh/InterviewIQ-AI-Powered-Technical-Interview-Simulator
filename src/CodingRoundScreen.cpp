#include "CodingRoundScreen.h"
#include "CodingJudge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFont>
#include <QSplitter>
#include <QFontDatabase>
#include <QApplication>

CodingRoundScreen::CodingRoundScreen(QWidget *parent)
    : QWidget(parent)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout();
    auto *backBtn = new QPushButton("← Back", this);
    connect(backBtn, &QPushButton::clicked, this, &CodingRoundScreen::backRequested);
    header->addWidget(backBtn);

    auto *title = new QLabel("Coding Round", this);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    header->addWidget(title);
    header->addStretch();

    m_difficultyFilterBox = new QComboBox(this);
    m_difficultyFilterBox->addItems({"All", "Easy", "Medium", "Hard"});
    connect(m_difficultyFilterBox, &QComboBox::currentIndexChanged,
            this, &CodingRoundScreen::onDifficultyFilterChanged);
    header->addWidget(new QLabel("Difficulty:", this));
    header->addWidget(m_difficultyFilterBox);

    m_problemBox = new QComboBox(this);
    connect(m_problemBox, &QComboBox::currentIndexChanged, this, &CodingRoundScreen::onProblemChanged);
    header->addWidget(new QLabel("Problem:", this));
    header->addWidget(m_problemBox);
    rootLayout->addLayout(header);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left: problem description
    m_descriptionLabel = new QLabel(splitter);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_descriptionLabel->setMargin(8);
    auto *descScroll = new QWidget(splitter);
    auto *descLayout = new QVBoxLayout(descScroll);
    descLayout->addWidget(m_descriptionLabel);
    descLayout->addStretch();
    splitter->addWidget(descScroll);

    // Right: editor + results, stacked
    auto *rightSide = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(rightSide);

    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_codeEditor = new QTextEdit(rightSide);
    m_codeEditor->setFont(monoFont);
    m_codeEditor->setLineWrapMode(QTextEdit::NoWrap);
    rightLayout->addWidget(m_codeEditor, /*stretch=*/3);

    auto *buttonRow = new QHBoxLayout();
    m_runSampleBtn = new QPushButton("Run Sample Tests", rightSide);
    connect(m_runSampleBtn, &QPushButton::clicked, this, &CodingRoundScreen::onRunSampleClicked);
    buttonRow->addWidget(m_runSampleBtn);

    m_submitBtn = new QPushButton("Submit", rightSide);
    connect(m_submitBtn, &QPushButton::clicked, this, &CodingRoundScreen::onSubmitClicked);
    buttonRow->addWidget(m_submitBtn);
    rightLayout->addLayout(buttonRow);

    m_statusLabel = new QLabel(rightSide);
    rightLayout->addWidget(m_statusLabel);

    m_resultsView = new QPlainTextEdit(rightSide);
    m_resultsView->setFont(monoFont);
    m_resultsView->setReadOnly(true);
    m_resultsView->setPlaceholderText("Run sample tests or submit to see results here.");
    rightLayout->addWidget(m_resultsView, /*stretch=*/2);

    splitter->addWidget(rightSide);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter);

    populateProblemList();
}

void CodingRoundScreen::onDifficultyFilterChanged(int /*filterIndex*/)
{
    populateProblemList();
}

void CodingRoundScreen::populateProblemList()
{
    const QString filter = m_difficultyFilterBox->currentText();

    // Block signals while rebuilding so onProblemChanged doesn't fire on
    // every addItem() call with a stale m_visibleIndices.
    m_problemBox->blockSignals(true);
    m_problemBox->clear();
    m_visibleIndices.clear();

    const auto &all = CodingProblemBank::all();
    for (int i = 0; i < all.size(); ++i) {
        if (filter == "All" || all.at(i).difficulty == filter) {
            m_problemBox->addItem(QString("[%1] %2").arg(all.at(i).difficulty, all.at(i).title));
            m_visibleIndices.append(i);
        }
    }
    m_problemBox->blockSignals(false);

    onProblemChanged(m_visibleIndices.isEmpty() ? -1 : 0);
}

void CodingRoundScreen::onProblemChanged(int comboIndex)
{
    if (comboIndex < 0 || comboIndex >= m_visibleIndices.size()) {
        m_descriptionLabel->setText("No problems match this difficulty filter.");
        m_codeEditor->clear();
        m_resultsView->clear();
        m_statusLabel->clear();
        return;
    }
    const CodingProblem &problem = CodingProblemBank::all().at(m_visibleIndices.at(comboIndex));

    QString desc = "<b>[" + problem.difficulty.toHtmlEscaped() + "] " + problem.title.toHtmlEscaped() + "</b><br><br>" +
                   problem.description.toHtmlEscaped().replace("\n", "<br>");
    desc += "<br><br><b>Sample tests:</b>";
    for (const CodingTestCase &test : problem.sampleTests) {
        desc += "<br>" + formatTestCase(test).toHtmlEscaped().replace("\n", "<br>");
    }
    m_descriptionLabel->setText(desc);

    m_codeEditor->setPlainText(problem.starterCode);
    m_resultsView->clear();
    m_statusLabel->clear();
}

QString CodingRoundScreen::formatTestCase(const CodingTestCase &test) const
{
    return QString("input: %1  →  expected: %2").arg(test.input.simplified(), test.expectedOutput.simplified());
}

void CodingRoundScreen::onRunSampleClicked()
{
    runAndShowResults(/*includeHidden=*/false);
}

void CodingRoundScreen::onSubmitClicked()
{
    runAndShowResults(/*includeHidden=*/true);
}

void CodingRoundScreen::runAndShowResults(bool includeHidden)
{
    const int comboIndex = m_problemBox->currentIndex();
    if (comboIndex < 0 || comboIndex >= m_visibleIndices.size()) return;
    const CodingProblem &problem = CodingProblemBank::all().at(m_visibleIndices.at(comboIndex));

    QVector<CodingTestCase> tests = problem.sampleTests;
    if (includeHidden) tests += problem.hiddenTests;

    m_runSampleBtn->setEnabled(false);
    m_submitBtn->setEnabled(false);
    m_statusLabel->setText("Compiling and running...");
    // Give Qt a chance to repaint the "Compiling..." status before the
    // blocking compile/run call below.
    qApp->processEvents();

    const CodingRunReport report = CodingJudge::runSubmission(m_codeEditor->toPlainText(), tests);

    m_runSampleBtn->setEnabled(true);
    m_submitBtn->setEnabled(true);

    if (!report.compileSucceeded) {
        m_statusLabel->setText("Compile error ✗");
        m_statusLabel->setStyleSheet("color: red;");
        m_resultsView->setPlainText(report.compileError);
        return;
    }

    int passed = 0;
    QString output;
    for (int i = 0; i < report.results.size(); ++i) {
        const CodingTestResult &result = report.results.at(i);
        if (result.passed) ++passed;
        output += QString("Test %1: %2  (%3 ms)\n")
                      .arg(i + 1)
                      .arg(result.passed ? "PASS ✓" : "FAIL ✗")
                      .arg(result.elapsedMs);
        if (!result.passed) {
            output += QString("  expected: %1\n  got:      %2\n")
                          .arg(tests.at(i).expectedOutput.trimmed(), result.actualOutput.trimmed());
        }
    }
    output += QString("\n%1 / %2 passed  •  total time: %3 ms")
                  .arg(passed).arg(report.results.size()).arg(report.totalElapsedMs);
    m_resultsView->setPlainText(output);

    const bool allPassed = (passed == report.results.size());
    m_statusLabel->setText(allPassed ? "All tests passed ✓" : QString("%1 / %2 passed").arg(passed).arg(report.results.size()));
    m_statusLabel->setStyleSheet(allPassed ? "color: #2a7a2a;" : "color: orange;");
}

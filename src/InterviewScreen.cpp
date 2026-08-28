#include "InterviewScreen.h"
#include "DatabaseManager.h"
#include "InterviewManager.h"
#include "CameraController.h"
#include "AudioRecorder.h"
#include "PythonBridge.h"
#include "ChartManager.h"
#include "PdfReportGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QVideoWidget>
#include <QProgressBar>
#include <QFont>
#include <QJsonArray>
#include <QStackedWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QStringList>

InterviewScreen::InterviewScreen(DatabaseManager *db, SettingsManager *settings, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    m_interviewManager = new InterviewManager(m_db, settings, this);
    m_camera = new CameraController(this);
    m_interviewManager->setCameraController(m_camera);

    // speech_to_text.py runs as its own long-lived server process (like the
    // camera CV scripts) so the Whisper model only loads once, not per answer.
    m_audioRecorder = new AudioRecorder(this);
    m_speechBridge = new PythonBridge("python/speech_to_text.py", this);
    m_speechBridge->start();

    connect(m_interviewManager, &InterviewManager::questionReady, this, &InterviewScreen::onQuestionReady);
    connect(m_interviewManager, &InterviewManager::timeRemainingChanged, this, &InterviewScreen::onTimeRemainingChanged);
    connect(m_interviewManager, &InterviewManager::evaluationReady, this, &InterviewScreen::onEvaluationReady);
    connect(m_interviewManager, &InterviewManager::finalReportReady, this, &InterviewScreen::onFinalReportReady);
    connect(m_camera, &CameraController::faceStatusChanged, this, &InterviewScreen::onFaceStatusChanged);
    connect(m_camera, &CameraController::postureStatusChanged, this, &InterviewScreen::onPostureStatusChanged);
    connect(m_audioRecorder, &AudioRecorder::recordingFinished, this, &InterviewScreen::onRecordingFinished);
    connect(m_audioRecorder, &AudioRecorder::errorOccurred, this, &InterviewScreen::onAudioError);
    connect(m_speechBridge, &PythonBridge::resultReceived, this, &InterviewScreen::onTranscriptionResult);
    connect(m_speechBridge, &PythonBridge::errorOccurred, this, &InterviewScreen::onAudioError);

    m_rootLayout = new QVBoxLayout(this);

    // --- Interview page --------------------------------------------------
    m_interviewPage = new QWidget(this);
    auto *pageLayout = new QVBoxLayout(m_interviewPage);

    auto *statusRow = new QHBoxLayout();
    m_progressLabel = new QLabel("Question 0 / 0", m_interviewPage);
    m_timerLabel = new QLabel("--:--", m_interviewPage);
    m_timerLabel->setFont(QFont("Arial", 14, QFont::Bold));
    m_faceStatusLabel = new QLabel("Camera: waiting...", m_interviewPage);
    m_postureStatusLabel = new QLabel("Posture: waiting...", m_interviewPage);
    statusRow->addWidget(m_progressLabel);
    statusRow->addStretch();
    statusRow->addWidget(m_faceStatusLabel);
    statusRow->addStretch();
    statusRow->addWidget(m_postureStatusLabel);
    statusRow->addStretch();
    statusRow->addWidget(m_timerLabel);
    pageLayout->addLayout(statusRow);

    m_timeProgressBar = new QProgressBar(m_interviewPage);
    m_timeProgressBar->setTextVisible(false);
    pageLayout->addWidget(m_timeProgressBar);

    auto *bodyRow = new QHBoxLayout();

    auto *leftColumn = new QVBoxLayout();
    m_questionLabel = new QLabel("Preparing your interview...", m_interviewPage);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setFont(QFont("Arial", 16));
    m_questionLabel->setMinimumHeight(80);
    leftColumn->addWidget(m_questionLabel);

    m_answerEdit = new QTextEdit(m_interviewPage);
    m_answerEdit->setPlaceholderText(
        "Speak your answer with \"Record Answer\", or type it here manually.");
    leftColumn->addWidget(m_answerEdit);

    auto *answerButtonRow = new QHBoxLayout();

    m_recordButton = new QPushButton("🎙 Record Answer", m_interviewPage);
    connect(m_recordButton, &QPushButton::clicked, this, &InterviewScreen::onRecordButtonClicked);
    answerButtonRow->addWidget(m_recordButton);

    m_submitButton = new QPushButton("Submit Answer", m_interviewPage);
    connect(m_submitButton, &QPushButton::clicked, this, &InterviewScreen::onSubmitClicked);
    answerButtonRow->addWidget(m_submitButton);

    leftColumn->addLayout(answerButtonRow);

    m_feedbackLabel = new QLabel("", m_interviewPage);
    m_feedbackLabel->setWordWrap(true);
    m_feedbackLabel->setStyleSheet("color: #2a7a2a;");
    leftColumn->addWidget(m_feedbackLabel);

    bodyRow->addLayout(leftColumn, 2);

    m_cameraPreview = new QVideoWidget(m_interviewPage);
    m_cameraPreview->setMinimumSize(320, 240);
    m_camera->setPreviewWidget(m_cameraPreview);
    bodyRow->addWidget(m_cameraPreview, 1);

    pageLayout->addLayout(bodyRow);

    // --- Summary page (built dynamically in showSummaryView) -------------
    m_summaryPage = new QWidget(this);

    m_rootLayout->addWidget(m_interviewPage);
    m_rootLayout->addWidget(m_summaryPage);
    m_summaryPage->hide();
}

InterviewScreen::~InterviewScreen()
{
    if (m_audioRecorder->isRecording()) m_audioRecorder->stopRecording();
    m_speechBridge->stop();
}

void InterviewScreen::beginInterview(int userId, const QString &category, const QString &difficulty, int durationMinutes)
{
    m_interviewPage->show();
    m_summaryPage->hide();

    m_totalSeconds = durationMinutes * 60;
    m_timeProgressBar->setRange(0, m_totalSeconds);
    m_timeProgressBar->setValue(m_totalSeconds);

    m_answerEdit->clear();
    m_feedbackLabel->clear();
    m_recordButton->setEnabled(true);
    m_recordButton->setText("🎙 Record Answer");

    m_interviewManager->configure(userId, category, difficulty, durationMinutes);
    m_interviewManager->start();
}

void InterviewScreen::onQuestionReady(const QString &question, int questionNumber, int totalQuestions)
{
    m_questionLabel->setText(question);
    m_progressLabel->setText(QString("Question %1 / %2").arg(questionNumber).arg(totalQuestions));
    m_answerEdit->clear();
    m_answerEdit->setEnabled(true);
    m_submitButton->setEnabled(true);
    m_recordButton->setEnabled(true);
    m_recordButton->setText("🎙 Record Answer");
    m_feedbackLabel->clear();
    m_lastAnswerWpm = 0.0;
}

void InterviewScreen::onTimeRemainingChanged(int secondsRemaining)
{
    secondsRemaining = qMax(0, secondsRemaining);
    const int minutes = secondsRemaining / 60;
    const int seconds = secondsRemaining % 60;
    m_timerLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
    m_timeProgressBar->setValue(secondsRemaining);
}

void InterviewScreen::onFaceStatusChanged(bool present, bool multipleFaces)
{
    if (multipleFaces) {
        m_faceStatusLabel->setText("Camera: multiple faces detected ⚠️");
        m_faceStatusLabel->setStyleSheet("color: orange;");
    } else if (!present) {
        m_faceStatusLabel->setText("Camera: face not detected ⚠️");
        m_faceStatusLabel->setStyleSheet("color: red;");
    } else {
        m_faceStatusLabel->setText("Camera: face detected ✓");
        m_faceStatusLabel->setStyleSheet("color: #2a7a2a;");
    }
}

void InterviewScreen::onPostureStatusChanged(const QString &leaning, bool headDown, bool excessiveMovement)
{
    if (leaning != "none" || headDown || excessiveMovement) {
        QStringList issues;
        if (leaning != "none") issues << QString("leaning %1").arg(leaning);
        if (headDown) issues << "head down";
        if (excessiveMovement) issues << "too much movement";
        m_postureStatusLabel->setText("Posture: " + issues.join(", ") + " ⚠️");
        m_postureStatusLabel->setStyleSheet("color: orange;");
    } else {
        m_postureStatusLabel->setText("Posture: good ✓");
        m_postureStatusLabel->setStyleSheet("color: #2a7a2a;");
    }
}

void InterviewScreen::onSubmitClicked()
{
    const QString answer = m_answerEdit->toPlainText().trimmed();
    if (answer.isEmpty()) return;

    m_answerEdit->setEnabled(false);
    m_submitButton->setEnabled(false);
    m_recordButton->setEnabled(false);
    m_feedbackLabel->setText("Evaluating your answer...");

    m_interviewManager->submitAnswerText(answer, m_lastAnswerWpm);
}

void InterviewScreen::onRecordButtonClicked()
{
    if (!m_audioRecorder->isRecording()) {
        // Starting a fresh recording discards whatever was typed manually.
        m_answerEdit->clear();
        m_answerEdit->setEnabled(false);
        m_submitButton->setEnabled(false);
        m_lastAnswerWpm = 0.0;

        if (m_audioRecorder->startRecording()) {
            m_recordButton->setText("⏹ Stop Recording");
            m_feedbackLabel->setText("Recording... click again when you're done answering.");
        } else {
            // startRecording() already emitted errorOccurred -> onAudioError,
            // just restore the typed-answer fallback.
            m_answerEdit->setEnabled(true);
            m_submitButton->setEnabled(true);
        }
    } else {
        m_recordButton->setEnabled(false);
        m_recordButton->setText("🎙 Record Answer");
        m_feedbackLabel->setText("Finishing recording...");
        m_audioRecorder->stopRecording();
    }
}

void InterviewScreen::onRecordingFinished(const QString &filePath)
{
    m_feedbackLabel->setText("Transcribing your answer...");
    m_speechBridge->sendRequest(QJsonObject{{"cmd", "transcribe"}, {"audio_path", filePath}});
}

void InterviewScreen::onTranscriptionResult(const QJsonObject &result)
{
    if (result.contains("error")) {
        m_feedbackLabel->setText("Transcription failed: " + result.value("error").toString() +
                                  " — you can type your answer instead.");
        m_answerEdit->setEnabled(true);
        m_submitButton->setEnabled(true);
        m_recordButton->setEnabled(true);
        return;
    }

    const QString transcript = result.value("text").toString().trimmed();
    m_answerEdit->setEnabled(true);
    m_recordButton->setEnabled(true);
    m_answerEdit->setPlainText(transcript);

    if (transcript.isEmpty()) {
        m_feedbackLabel->setText("Didn't catch any speech — try recording again, or type your answer.");
        m_submitButton->setEnabled(true);
        return;
    }

    // Speaking-style metrics: words_per_minute is captured for the average
    // shown in the final report; pause_count/filler_count are also in
    // `result` if a future report screen wants to surface them too.
    m_lastAnswerWpm = result.value("words_per_minute").toDouble(0.0);
    m_feedbackLabel->setText("Transcribed. Review the text below, then Submit Answer.");
    m_submitButton->setEnabled(true);
}

void InterviewScreen::onAudioError(const QString &message)
{
    m_feedbackLabel->setText("Voice input error: " + message + " — you can type your answer instead.");
    m_answerEdit->setEnabled(true);
    m_submitButton->setEnabled(true);
    m_recordButton->setEnabled(true);
    m_recordButton->setText("🎙 Record Answer");
}

void InterviewScreen::onEvaluationReady(const QJsonObject &evaluation)
{
    if (evaluation.contains("error")) {
        m_feedbackLabel->setText("Evaluation unavailable: " + evaluation.value("error").toString());
        return;
    }
    m_feedbackLabel->setText(QString("Accuracy %1/10 · Completeness %2/10 · Clarity %3/10 — %4")
                                  .arg(evaluation.value("accuracy").toInt())
                                  .arg(evaluation.value("completeness").toInt())
                                  .arg(evaluation.value("clarity").toInt())
                                  .arg(evaluation.value("summary").toString()));
}

void InterviewScreen::onFinalReportReady(const QJsonObject &report)
{
    showSummaryView(report);
}

void InterviewScreen::showSummaryView(const QJsonObject &report)
{
    // Rebuild the summary page fresh each time.
    qDeleteAll(m_summaryPage->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
    delete m_summaryPage->layout();

    auto *layout = new QVBoxLayout(m_summaryPage);

    auto *title = new QLabel("Interview Complete 🎉", m_summaryPage);
    title->setFont(QFont("Arial", 20, QFont::Bold));
    layout->addWidget(title);

    layout->addWidget(new QLabel(QString("Category: %1  •  Difficulty: %2")
                                      .arg(report.value("category").toString())
                                      .arg(report.value("difficulty").toString()), m_summaryPage));
    layout->addWidget(new QLabel(QString("Eye contact: %1%  •  Dominant emotion: %2")
                                      .arg(report.value("eyeContactPct").toDouble(), 0, 'f', 0)
                                      .arg(report.value("dominantEmotion").toString()), m_summaryPage));

    layout->addWidget(new QLabel(QString("Posture: leaning %1% of the time  •  head down %2%  •  excessive movement %3%")
                                      .arg(report.value("leaningPct").toDouble(), 0, 'f', 0)
                                      .arg(report.value("headDownPct").toDouble(), 0, 'f', 0)
                                      .arg(report.value("excessiveMovementPct").toDouble(), 0, 'f', 0), m_summaryPage));

    const QJsonArray suggestions = report.value("aiSuggestions").toArray();
    if (!suggestions.isEmpty()) {
        auto *suggestTitle = new QLabel("Suggested review topics:", m_summaryPage);
        suggestTitle->setFont(QFont("Arial", 12, QFont::Bold));
        layout->addWidget(suggestTitle);
        for (const auto &s : suggestions) {
            layout->addWidget(new QLabel("• " + s.toString(), m_summaryPage));
        }
    }

    auto *buttonRow = new QHBoxLayout();

    auto *exportBtn = new QPushButton("Download PDF Report", m_summaryPage);
    connect(exportBtn, &QPushButton::clicked, this, [this, report]() {
        const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        const QString defaultPath = QDir(defaultDir).filePath("InterviewIQ_Report.pdf");
        const QString path = QFileDialog::getSaveFileName(this, "Save Interview Report", defaultPath, "PDF Files (*.pdf)");
        if (path.isEmpty()) return;

        // Render the score-breakdown pie chart off-screen and grab it as a
        // pixmap to embed in the PDF.
        double technical = 0, communication = 0, confidence = 0;
        const QJsonArray perQuestion = report.value("perQuestion").toArray();
        if (!perQuestion.isEmpty()) {
            double accSum = 0, compSum = 0, clarSum = 0;
            for (const auto &q : perQuestion) {
                accSum += q.toObject().value("accuracy").toInt();
                compSum += q.toObject().value("completeness").toInt();
                clarSum += q.toObject().value("clarity").toInt();
            }
            const int n = perQuestion.size();
            technical = ((accSum / n) + (compSum / n)) / 2.0 * 10.0;
            communication = (clarSum / n) * 10.0;
            confidence = report.value("eyeContactPct").toDouble();
        }

        auto *chartView = ChartManager::buildScoreBreakdownPie(technical, communication, confidence, nullptr, /*darkTheme=*/false);
        chartView->resize(600, 400);
        const QPixmap chartPixmap = chartView->grab();
        delete chartView;

        QJsonObject pdfReport = report;
        pdfReport["technicalScore"] = technical;
        pdfReport["communicationScore"] = communication;
        pdfReport["confidenceScore"] = confidence;
        pdfReport["overallRating"] = QString("%1 / 100").arg((technical + communication + confidence) / 3.0, 0, 'f', 0);

        if (PdfReportGenerator::generate(pdfReport, path, chartPixmap)) {
            QMessageBox::information(this, "Report Saved", "Your interview report was saved to:\n" + path);
        } else {
            QMessageBox::warning(this, "Export Failed", "Could not save the PDF report.");
        }
    });
    buttonRow->addWidget(exportBtn);

    auto *doneBtn = new QPushButton("Back to Dashboard", m_summaryPage);
    const int finishedInterviewId = m_interviewManager->interviewId();
    connect(doneBtn, &QPushButton::clicked, this, [this, report, finishedInterviewId]() {
        emit interviewCompleted(finishedInterviewId, report);
    });
    buttonRow->addWidget(doneBtn);
    layout->addLayout(buttonRow);
    layout->addStretch();

    m_interviewPage->hide();
    m_summaryPage->show();
}

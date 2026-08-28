#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

class DatabaseManager;
class CameraController;
class PythonBridge;
class SettingsManager;

/**
 * Drives the end-to-end interview workflow described in the spec's
 * "System Workflow" diagram:
 *
 *   Select category/difficulty -> init camera/mic -> AI generates questions
 *   -> user answers (voice/camera pipelines run concurrently) -> AI evaluates
 *   -> charts -> PDF report -> save to DB -> dashboard updates.
 */
class InterviewManager : public QObject {
    Q_OBJECT
public:
    enum class State {
        Idle,
        Initializing,
        AskingQuestion,
        AwaitingAnswer,
        Evaluating,
        Finished
    };
    Q_ENUM(State)

    struct QuestionResult {
        QString question;
        QString answerText;
        int accuracy = 0;
        int completeness = 0;
        int clarity = 0;
        int confidence = 0;
        QString summary;
    };

    explicit InterviewManager(DatabaseManager *db, SettingsManager *settings, QObject *parent = nullptr);

    // Give the manager a camera to drive during the interview (owned by the
    // caller — typically the interview screen widget). Pass nullptr to run
    // without camera monitoring (e.g. for quick testing).
    void setCameraController(CameraController *camera) { m_camera = camera; }

    void configure(int userId, const QString &category, const QString &difficulty, int durationMinutes);
    void start();
    // speakingWpm: pass the words-per-minute measured by speech_to_text.py
    // for a voice-recorded answer, or 0.0 for a typed one (typed answers
    // aren't counted toward the interview's average WPM).
    void submitAnswerText(const QString &transcribedAnswer, double speakingWpm = 0.0);
    void stop();

    State state() const { return m_state; }
    int interviewId() const { return m_interviewId; }
    int questionCount() const { return m_questionQueue.size(); }
    int currentQuestionNumber() const { return m_currentQuestionIndex + 1; }
    const QVector<QuestionResult> &results() const { return m_results; }

signals:
    void stateChanged(State state);
    void questionReady(const QString &questionText, int questionNumber, int totalQuestions);
    void evaluationReady(const QJsonObject &evaluation);
    void interviewFinished(int interviewId);
    void timeRemainingChanged(int secondsRemaining);
    void finalReportReady(const QJsonObject &report);

private slots:
    void onTick();
    void onLlmResult(const QJsonObject &result);
    void onRecommendationResult(const QJsonObject &result);

private:
    void setState(State s);
    void askNextQuestion();
    void finishInterview();
    QJsonObject buildFinalReport(const QJsonArray &extraSuggestions = {}) const;

    DatabaseManager *m_db;
    SettingsManager *m_settings;
    CameraController *m_camera = nullptr;
    PythonBridge *m_llmBridge = nullptr;
    PythonBridge *m_recommendationBridge = nullptr;

    int m_userId = -1;
    QString m_category;
    QString m_difficulty;
    int m_durationSeconds = 0;
    int m_secondsElapsed = 0;
    int m_interviewId = -1;

    QStringList m_questionQueue;
    int m_currentQuestionIndex = -1;
    QString m_pendingAnswer;
    QVector<double> m_wpmSamples; // one entry per voice-recorded answer

    QVector<QuestionResult> m_results;

    State m_state = State::Idle;
    QTimer m_countdownTimer;
};

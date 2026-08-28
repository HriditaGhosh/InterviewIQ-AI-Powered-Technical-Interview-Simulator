#pragma once

#include <QWidget>
#include <QJsonObject>

class DatabaseManager;
class SettingsManager;
class InterviewManager;
class CameraController;
class AudioRecorder;
class PythonBridge;
class QLabel;
class QTextEdit;
class QPushButton;
class QVideoWidget;
class QProgressBar;
class QVBoxLayout;

/**
 * The live interview screen: shows the current question, a camera preview,
 * a countdown timer, a "Record Answer" button that captures the reply via
 * microphone and transcribes it (with a typed QTextEdit kept as a manual
 * fallback), and per-question AI feedback as it comes in.
 *
 * Voice flow: AudioRecorder records to a temp .wav -> on
 * recordingFinished(path), a PythonBridge running python/speech_to_text.py
 * is sent {"cmd": "transcribe", "audio_path": path} -> the returned
 * transcript is dropped into m_answerEdit and passed to
 * InterviewManager::submitAnswerText(), exactly like the typed path.
 * InterviewManager itself does not need to change.
 */
class InterviewScreen : public QWidget {
    Q_OBJECT
public:
    explicit InterviewScreen(DatabaseManager *db, SettingsManager *settings, QWidget *parent = nullptr);
    ~InterviewScreen() override;

    // Configure and begin a new interview.
    void beginInterview(int userId, const QString &category, const QString &difficulty, int durationMinutes);

signals:
    void interviewCompleted(int interviewId, const QJsonObject &finalReport);
    void exitRequested();

private slots:
    void onQuestionReady(const QString &question, int questionNumber, int totalQuestions);
    void onTimeRemainingChanged(int secondsRemaining);
    void onEvaluationReady(const QJsonObject &evaluation);
    void onFinalReportReady(const QJsonObject &report);
    void onSubmitClicked();
    void onFaceStatusChanged(bool present, bool multipleFaces);
    void onPostureStatusChanged(const QString &leaning, bool headDown, bool excessiveMovement);
    void onRecordButtonClicked();
    void onRecordingFinished(const QString &filePath);
    void onTranscriptionResult(const QJsonObject &result);
    void onAudioError(const QString &message);

private:
    void showSummaryView(const QJsonObject &report);

    DatabaseManager *m_db;
    InterviewManager *m_interviewManager;
    CameraController *m_camera;
    AudioRecorder *m_audioRecorder;
    PythonBridge *m_speechBridge;

    QLabel *m_questionLabel = nullptr;
    QLabel *m_progressLabel = nullptr;
    QLabel *m_timerLabel = nullptr;
    QLabel *m_faceStatusLabel = nullptr;
    QLabel *m_postureStatusLabel = nullptr;
    QLabel *m_feedbackLabel = nullptr;
    QTextEdit *m_answerEdit = nullptr;
    QPushButton *m_submitButton = nullptr;
    QPushButton *m_recordButton = nullptr;
    QVideoWidget *m_cameraPreview = nullptr;
    QProgressBar *m_timeProgressBar = nullptr;
    QVBoxLayout *m_rootLayout = nullptr;
    QWidget *m_interviewPage = nullptr;
    QWidget *m_summaryPage = nullptr;

    int m_totalSeconds = 0;
    double m_lastAnswerWpm = 0.0; // set by onTranscriptionResult, consumed (and reset) on submit
};

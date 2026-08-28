#pragma once

#include <QObject>
#include <QAudioInput>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QString>

/**
 * Records the candidate's microphone input to a single-answer .wav file
 * using Qt Multimedia (QAudioInput + QMediaCaptureSession + QMediaRecorder).
 *
 * This is the piece InterviewScreen.h describes as the "real voice input"
 * upgrade: InterviewScreen owns one of these, starts/stops it around a
 * "Record Answer" button, and on recordingFinished() hands the resulting
 * .wav path to a PythonBridge running python/speech_to_text.py. The
 * returned transcript is then passed to InterviewManager::submitAnswerText()
 * exactly as the typed answer is today — InterviewManager itself doesn't
 * need to change.
 *
 * Usage:
 *   auto *recorder = new AudioRecorder(this);
 *   connect(recorder, &AudioRecorder::recordingFinished, this, &MyClass::onRecordingFinished);
 *   recorder->startRecording();
 *   ...
 *   recorder->stopRecording(); // recordingFinished(path) fires once flushed
 */
class AudioRecorder : public QObject {
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);

    // Begin recording to a fresh temp .wav file. Returns false (and emits
    // errorOccurred) if the underlying recorder could not start — e.g. no
    // microphone is available/permitted.
    bool startRecording();

    // Stop recording. recordingFinished(filePath) fires once the file is
    // flushed to disk and ready to hand off to speech_to_text.py. If no
    // recording is in progress, this is a no-op.
    void stopRecording();

    bool isRecording() const;
    QString lastFilePath() const { return m_currentFilePath; }

signals:
    void recordingFinished(const QString &filePath);
    void errorOccurred(const QString &message);

private slots:
    void onRecorderStateChanged(QMediaRecorder::RecorderState state);
    void onRecorderErrorOccurred(QMediaRecorder::Error error, const QString &errorString);

private:
    QAudioInput m_audioInput;
    QMediaCaptureSession m_captureSession;
    QMediaRecorder m_recorder;
    QString m_currentFilePath;
    bool m_pendingFinish = false;
};

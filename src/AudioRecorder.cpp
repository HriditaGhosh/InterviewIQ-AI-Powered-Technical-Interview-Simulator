#include "AudioRecorder.h"

#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMediaFormat>
#include <QUrl>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent)
{
    m_captureSession.setAudioInput(&m_audioInput);
    m_captureSession.setRecorder(&m_recorder);

    // Plain WAV (PCM) so speech_to_text.py / Whisper can read it directly
    // without a transcoding step.
    QMediaFormat format(QMediaFormat::FileFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    m_recorder.setMediaFormat(format);
    m_recorder.setQuality(QMediaRecorder::HighQuality);

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged,
            this, &AudioRecorder::onRecorderStateChanged);
    connect(&m_recorder, &QMediaRecorder::errorOccurred,
            this, &AudioRecorder::onRecorderErrorOccurred);
}

bool AudioRecorder::startRecording()
{
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(tempDir);
    m_currentFilePath = QDir(tempDir).filePath(
        QString("interviewiq_answer_%1.wav").arg(QDateTime::currentMSecsSinceEpoch()));

    m_recorder.setOutputLocation(QUrl::fromLocalFile(m_currentFilePath));
    m_pendingFinish = false;
    m_recorder.record();

    if (m_recorder.error() != QMediaRecorder::NoError) {
        emit errorOccurred(m_recorder.errorString());
        return false;
    }
    return true;
}

void AudioRecorder::stopRecording()
{
    if (m_recorder.recorderState() != QMediaRecorder::RecordingState) return;

    // recordingFinished() is emitted from onRecorderStateChanged() once the
    // recorder confirms it has actually stopped and flushed the file, not
    // immediately here — writing can lag the stop() call slightly.
    m_pendingFinish = true;
    m_recorder.stop();
}

bool AudioRecorder::isRecording() const
{
    return m_recorder.recorderState() == QMediaRecorder::RecordingState;
}

void AudioRecorder::onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::StoppedState && m_pendingFinish) {
        m_pendingFinish = false;
        emit recordingFinished(m_currentFilePath);
    }
}

void AudioRecorder::onRecorderErrorOccurred(QMediaRecorder::Error error, const QString &errorString)
{
    Q_UNUSED(error);
    m_pendingFinish = false;
    emit errorOccurred(errorString);
}

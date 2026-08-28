#pragma once

#include <QObject>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QVideoSink>
#include <QVideoFrame>
#include <QTimer>
#include <QMap>
#include <QJsonObject>

class PythonBridge;

/**
 * Owns the QCamera/QMediaCaptureSession, previews the live feed, and
 * periodically samples a frame to send to the three Python CV pipelines
 * (face_detection.py, eye_tracking.py, emotion_detection.py) over
 * PythonBridge, aggregating their results for the interview report.
 *
 * Sampling runs at a low rate (default 1 frame / 2 seconds) — analyzing
 * every video frame would be wasteful and isn't necessary for interview-
 * level metrics like eye-contact % or dominant emotion.
 */
class CameraController : public QObject {
    Q_OBJECT
public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController() override;

    // Attach a QVideoWidget from the UI to preview the live feed.
    void setPreviewWidget(QVideoWidget *widget);

    bool start();
    void stop();

    // Call at the start of each interview so aggregates reflect only that
    // session, not any prior camera usage.
    void resetAggregates();

    // --- Aggregate results, valid after at least one sample --------------
    bool facePresent() const { return m_lastFacePresent; }
    bool multipleFacesDetected() const { return m_anyMultipleFaces; }
    double eyeContactPercentage() const;
    QString dominantEmotion() const;
    int samplesTaken() const { return m_framesSampled; }

    // Posture (spec module 8): percentage of samples where the candidate
    // was leaning, had their head down, or moved excessively.
    double leaningPercentage() const;
    double headDownPercentage() const;
    double excessiveMovementPercentage() const;

signals:
    void faceStatusChanged(bool present, bool multipleFaces);
    void eyeContactSampled(bool lookingAtCamera);
    void emotionSampled(const QString &emotion);
    void postureStatusChanged(const QString &leaning, bool headDown, bool excessiveMovement);
    void errorOccurred(const QString &message);

private slots:
    void onVideoFrameChanged(const QVideoFrame &frame);
    void onSampleTimerTick();
    void onFaceResult(const QJsonObject &result);
    void onEyeResult(const QJsonObject &result);
    void onEmotionResult(const QJsonObject &result);
    void onPostureResult(const QJsonObject &result);

private:
    void dispatchFrame(const QString &framePath);

    QCamera m_camera;
    QMediaCaptureSession m_captureSession;
    QVideoSink m_videoSink;
    QVideoFrame m_lastFrame;
    bool m_hasFrame = false;

    QTimer m_sampleTimer;
    int m_sampleIntervalMs = 2000;

    PythonBridge *m_faceBridge = nullptr;
    PythonBridge *m_eyeBridge = nullptr;
    PythonBridge *m_emotionBridge = nullptr;
    PythonBridge *m_postureBridge = nullptr;

    // Aggregates for the current interview session
    int m_framesSampled = 0;
    int m_framesLookingAtCamera = 0;
    bool m_lastFacePresent = false;
    bool m_anyMultipleFaces = false;
    QMap<QString, int> m_emotionCounts;

    int m_postureSamples = 0;
    int m_leaningSamples = 0;
    int m_headDownSamples = 0;
    int m_excessiveMovementSamples = 0;
};

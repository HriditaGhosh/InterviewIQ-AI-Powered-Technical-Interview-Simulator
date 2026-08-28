#include "CameraController.h"
#include "PythonBridge.h"

#include <QDir>
#include <QStandardPaths>
#include <QImage>
#include <QDebug>

CameraController::CameraController(QObject *parent)
    : QObject(parent)
{
    m_captureSession.setCamera(&m_camera);
    m_captureSession.setVideoSink(&m_videoSink);

    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this, &CameraController::onVideoFrameChanged);
    connect(&m_sampleTimer, &QTimer::timeout, this, &CameraController::onSampleTimerTick);
    m_sampleTimer.setInterval(m_sampleIntervalMs);

    // Each Python CV script runs as its own long-lived server process so
    // model/cascade loading only happens once, not per-frame.
    m_faceBridge = new PythonBridge("python/face_detection.py", this);
    m_eyeBridge = new PythonBridge("python/eye_tracking.py", this);
    m_emotionBridge = new PythonBridge("python/emotion_detection.py", this);
    m_postureBridge = new PythonBridge("python/posture_detection.py", this);

    connect(m_faceBridge, &PythonBridge::resultReceived, this, &CameraController::onFaceResult);
    connect(m_eyeBridge, &PythonBridge::resultReceived, this, &CameraController::onEyeResult);
    connect(m_emotionBridge, &PythonBridge::resultReceived, this, &CameraController::onEmotionResult);
    connect(m_postureBridge, &PythonBridge::resultReceived, this, &CameraController::onPostureResult);

    connect(m_faceBridge, &PythonBridge::errorOccurred, this, &CameraController::errorOccurred);
    connect(m_eyeBridge, &PythonBridge::errorOccurred, this, &CameraController::errorOccurred);
    connect(m_emotionBridge, &PythonBridge::errorOccurred, this, &CameraController::errorOccurred);
    connect(m_postureBridge, &PythonBridge::errorOccurred, this, &CameraController::errorOccurred);
}

CameraController::~CameraController()
{
    stop();
}

void CameraController::setPreviewWidget(QVideoWidget *widget)
{
    m_captureSession.setVideoOutput(widget);
}

bool CameraController::start()
{
    m_faceBridge->start();
    m_eyeBridge->start();
    m_emotionBridge->start();
    m_postureBridge->start();

    resetAggregates();

    m_camera.start();
    if (!m_camera.isActive()) {
        emit errorOccurred("Failed to start camera — check that a webcam is connected and permitted.");
        return false;
    }

    m_sampleTimer.start();
    return true;
}

void CameraController::stop()
{
    m_sampleTimer.stop();
    m_camera.stop();
    m_faceBridge->stop();
    m_eyeBridge->stop();
    m_emotionBridge->stop();
    m_postureBridge->stop();
}

void CameraController::resetAggregates()
{
    m_framesSampled = 0;
    m_framesLookingAtCamera = 0;
    m_lastFacePresent = false;
    m_anyMultipleFaces = false;
    m_emotionCounts.clear();

    m_postureSamples = 0;
    m_leaningSamples = 0;
    m_headDownSamples = 0;
    m_excessiveMovementSamples = 0;
    m_postureBridge->sendRequest(QJsonObject{{"cmd", "reset"}});
}

double CameraController::eyeContactPercentage() const
{
    if (m_framesSampled == 0) return 0.0;
    return (double(m_framesLookingAtCamera) / double(m_framesSampled)) * 100.0;
}

QString CameraController::dominantEmotion() const
{
    if (m_emotionCounts.isEmpty()) return "Neutral";

    QString best = "Neutral";
    int bestCount = -1;
    for (auto it = m_emotionCounts.constBegin(); it != m_emotionCounts.constEnd(); ++it) {
        if (it.value() > bestCount) {
            bestCount = it.value();
            best = it.key();
        }
    }
    return best;
}

void CameraController::onVideoFrameChanged(const QVideoFrame &frame)
{
    // Just cache the latest frame; the sample timer decides when to act on
    // it, so we don't run CV analysis at full camera frame rate.
    m_lastFrame = frame;
    m_hasFrame = frame.isValid();
}

void CameraController::onSampleTimerTick()
{
    if (!m_hasFrame) return;

    QImage image = m_lastFrame.toImage();
    if (image.isNull()) return;

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString framePath = QDir(tempDir).filePath(
        QString("interviewiq_frame_%1.jpg").arg(m_framesSampled));

    if (!image.save(framePath, "JPG", 85)) {
        emit errorOccurred("Failed to write sampled camera frame to disk.");
        return;
    }

    ++m_framesSampled;
    dispatchFrame(framePath);
}

void CameraController::dispatchFrame(const QString &framePath)
{
    const QJsonObject request{{"cmd", "detect_frame"}, {"frame_path", framePath}};
    m_faceBridge->sendRequest(request);
    m_eyeBridge->sendRequest(request);
    m_emotionBridge->sendRequest(request);
    m_postureBridge->sendRequest(request);
}

void CameraController::onFaceResult(const QJsonObject &result)
{
    if (result.contains("error")) return;

    const bool present = result.value("face_present").toBool();
    const bool multiple = result.value("multiple_faces").toBool();

    m_lastFacePresent = present;
    if (multiple) m_anyMultipleFaces = true;

    emit faceStatusChanged(present, multiple);
}

void CameraController::onEyeResult(const QJsonObject &result)
{
    if (result.contains("error")) return;

    const bool lookingAtCamera = result.value("looking_at_camera").toBool();
    if (lookingAtCamera) ++m_framesLookingAtCamera;

    emit eyeContactSampled(lookingAtCamera);
}

void CameraController::onEmotionResult(const QJsonObject &result)
{
    if (result.contains("error")) return;

    const QString emotion = result.value("emotion").toString("Neutral");
    m_emotionCounts[emotion] = m_emotionCounts.value(emotion, 0) + 1;

    emit emotionSampled(emotion);
}

void CameraController::onPostureResult(const QJsonObject &result)
{
    if (result.contains("error")) return;
    if (!result.value("posture_present").toBool()) return; // no person detected this sample

    ++m_postureSamples;

    const QString leaning = result.value("leaning").toString("none");
    const bool headDown = result.value("head_down").toBool();
    const bool excessiveMovement = result.value("excessive_movement").toBool();

    if (leaning != "none") ++m_leaningSamples;
    if (headDown) ++m_headDownSamples;
    if (excessiveMovement) ++m_excessiveMovementSamples;

    emit postureStatusChanged(leaning, headDown, excessiveMovement);
}

double CameraController::leaningPercentage() const
{
    if (m_postureSamples == 0) return 0.0;
    return (double(m_leaningSamples) / double(m_postureSamples)) * 100.0;
}

double CameraController::headDownPercentage() const
{
    if (m_postureSamples == 0) return 0.0;
    return (double(m_headDownSamples) / double(m_postureSamples)) * 100.0;
}

double CameraController::excessiveMovementPercentage() const
{
    if (m_postureSamples == 0) return 0.0;
    return (double(m_excessiveMovementSamples) / double(m_postureSamples)) * 100.0;
}

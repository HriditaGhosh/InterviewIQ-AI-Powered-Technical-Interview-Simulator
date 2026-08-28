#pragma once

#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QJsonObject>
#include <QString>

/**
 * Runs a Python AI script as a subprocess and exchanges line-delimited JSON
 * over stdin/stdout. See docs/PROTOCOL.md for the message contract.
 *
 * Usage:
 *   auto *bridge = new PythonBridge("python/face_detection.py", this);
 *   connect(bridge, &PythonBridge::resultReceived, this, &MyClass::onFaceResult);
 *   bridge->start();
 *   bridge->sendRequest({{"cmd", "detect_frame"}, {"frame_path", path}});
 */
class PythonBridge : public QObject {
    Q_OBJECT
public:
    explicit PythonBridge(QString scriptPath, QObject *parent = nullptr);

    void start();
    void stop();
    void sendRequest(const QJsonObject &request);

    // Must be called before start(). Lets the C++ side control things like
    // OLLAMA_MODEL / AI_BACKEND for llm_feedback.py based on Settings,
    // without hardcoding a model name into the Python script.
    void setEnvironmentVariable(const QString &name, const QString &value);

signals:
    void resultReceived(const QJsonObject &result);
    void errorOccurred(const QString &message);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessError(QProcess::ProcessError error);

private:
    QString m_scriptPath;
    QProcess m_process;
    QByteArray m_stdoutBuffer;
    QProcessEnvironment m_extraEnv = QProcessEnvironment::systemEnvironment();
};

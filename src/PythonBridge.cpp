#include "PythonBridge.h"

#include <QJsonDocument>
#include <QDebug>

PythonBridge::PythonBridge(QString scriptPath, QObject *parent)
    : QObject(parent), m_scriptPath(std::move(scriptPath))
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &PythonBridge::onReadyReadStdout);
    connect(&m_process, &QProcess::readyReadStandardError, this, &PythonBridge::onReadyReadStderr);
    connect(&m_process, &QProcess::errorOccurred, this, &PythonBridge::onProcessError);
}

void PythonBridge::start()
{
#ifdef Q_OS_WIN
    // The official python.org Windows installer puts "python" (and "py")
    // on PATH, not "python3" — using "python3" here would fail to start on
    // every Windows machine, including a stock setup like this project's.
    const QString interpreter = "python";
#else
    const QString interpreter = "python3";
#endif

    m_process.setProcessEnvironment(m_extraEnv);
    m_process.start(interpreter, {m_scriptPath, "--serve"});
    if (!m_process.waitForStarted(3000)) {
        emit errorOccurred(QString("Failed to start %1").arg(m_scriptPath));
    }
}

void PythonBridge::setEnvironmentVariable(const QString &name, const QString &value)
{
    m_extraEnv.insert(name, value);
}

void PythonBridge::stop()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.closeWriteChannel();
        m_process.waitForFinished(2000);
        m_process.kill();
    }
}

void PythonBridge::sendRequest(const QJsonObject &request)
{
    if (m_process.state() != QProcess::Running) {
        emit errorOccurred("PythonBridge: process is not running");
        return;
    }
    const QByteArray line = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";
    m_process.write(line);
}

void PythonBridge::onReadyReadStdout()
{
    m_stdoutBuffer += m_process.readAllStandardOutput();

    int newlineIndex;
    while ((newlineIndex = m_stdoutBuffer.indexOf('\n')) != -1) {
        const QByteArray line = m_stdoutBuffer.left(newlineIndex);
        m_stdoutBuffer.remove(0, newlineIndex + 1);

        if (line.trimmed().isEmpty()) continue;

        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit errorOccurred(QString("PythonBridge: bad JSON from %1: %2")
                                    .arg(m_scriptPath, parseError.errorString()));
            continue;
        }
        emit resultReceived(doc.object());
    }
}

void PythonBridge::onReadyReadStderr()
{
    const QByteArray err = m_process.readAllStandardError();
    if (!err.isEmpty()) {
        qWarning().noquote() << "[" << m_scriptPath << "stderr]" << err;
    }
}

void PythonBridge::onProcessError(QProcess::ProcessError error)
{
    emit errorOccurred(QString("PythonBridge process error (%1): %2")
                            .arg(int(error))
                            .arg(m_process.errorString()));
}

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

/**
 * User-configurable settings: camera device, microphone device, theme,
 * language, AI model (e.g. llama3.2 vs mistral vs OpenAI), difficulty,
 * notification preferences. Backed by QSettings (INI file).
 */
class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString cameraDevice() const;
    void setCameraDevice(const QString &deviceId);

    QString microphoneDevice() const;
    void setMicrophoneDevice(const QString &deviceId);

    QString theme() const;              // "Light" | "Dark"
    void setTheme(const QString &theme);

    QString language() const;
    void setLanguage(const QString &language);

    QString aiModel() const;            // e.g. "llama3.2", "mistral", "gpt-4o-mini"
    void setAiModel(const QString &model);

    QString defaultDifficulty() const;
    void setDefaultDifficulty(const QString &difficulty);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

private:
    QSettings m_settings;
};

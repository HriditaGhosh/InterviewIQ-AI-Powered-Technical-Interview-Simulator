#include "SettingsManager.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), m_settings("InterviewIQ", "InterviewIQ")
{
}

QString SettingsManager::cameraDevice() const { return m_settings.value("camera/device").toString(); }
void SettingsManager::setCameraDevice(const QString &deviceId) { m_settings.setValue("camera/device", deviceId); }

QString SettingsManager::microphoneDevice() const { return m_settings.value("audio/device").toString(); }
void SettingsManager::setMicrophoneDevice(const QString &deviceId) { m_settings.setValue("audio/device", deviceId); }

QString SettingsManager::theme() const { return m_settings.value("ui/theme", "Light").toString(); }
void SettingsManager::setTheme(const QString &theme) { m_settings.setValue("ui/theme", theme); }

QString SettingsManager::language() const { return m_settings.value("ui/language", "English").toString(); }
void SettingsManager::setLanguage(const QString &language) { m_settings.setValue("ui/language", language); }

QString SettingsManager::aiModel() const { return m_settings.value("ai/model", "llama3.2").toString(); }
void SettingsManager::setAiModel(const QString &model) { m_settings.setValue("ai/model", model); }

QString SettingsManager::defaultDifficulty() const { return m_settings.value("interview/difficulty", "Medium").toString(); }
void SettingsManager::setDefaultDifficulty(const QString &difficulty) { m_settings.setValue("interview/difficulty", difficulty); }

bool SettingsManager::notificationsEnabled() const { return m_settings.value("notifications/enabled", true).toBool(); }
void SettingsManager::setNotificationsEnabled(bool enabled) { m_settings.setValue("notifications/enabled", enabled); }

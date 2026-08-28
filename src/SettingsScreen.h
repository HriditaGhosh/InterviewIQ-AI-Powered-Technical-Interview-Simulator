#pragma once

#include <QWidget>

class SettingsManager;
class QComboBox;
class QCheckBox;
class QLabel;

/**
 * The Settings screen (spec module 21): camera device, microphone device,
 * theme, language, AI model, default difficulty, notifications. Backed by
 * SettingsManager (QSettings). Selecting a theme here calls
 * ThemeManager::applyTheme() immediately, so Dark mode isn't just persisted
 * — it actually changes how the app looks, right away and on next launch.
 */
class SettingsScreen : public QWidget {
    Q_OBJECT
public:
    explicit SettingsScreen(SettingsManager *settings, QWidget *parent = nullptr);

signals:
    void backRequested();

private slots:
    void onSaveClicked();

private:
    void populateDeviceLists();
    void loadFromSettings();

    SettingsManager *m_settings;

    QComboBox *m_cameraDeviceBox = nullptr;
    QComboBox *m_micDeviceBox = nullptr;
    QComboBox *m_themeBox = nullptr;
    QComboBox *m_languageBox = nullptr;
    QComboBox *m_aiModelBox = nullptr;
    QComboBox *m_difficultyBox = nullptr;
    QCheckBox *m_notificationsCheck = nullptr;
    QLabel *m_savedLabel = nullptr;
};

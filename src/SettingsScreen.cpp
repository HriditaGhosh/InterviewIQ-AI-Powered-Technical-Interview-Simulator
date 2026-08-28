#include "SettingsScreen.h"
#include "SettingsManager.h"
#include "ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QAudioDevice>

SettingsScreen::SettingsScreen(SettingsManager *settings, QWidget *parent)
    : QWidget(parent), m_settings(settings)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *title = new QLabel("Settings", this);
    title->setFont(QFont("Arial", 20, QFont::Bold));
    rootLayout->addWidget(title);

    auto *form = new QFormLayout();

    m_cameraDeviceBox = new QComboBox(this);
    form->addRow("Camera", m_cameraDeviceBox);

    m_micDeviceBox = new QComboBox(this);
    form->addRow("Microphone", m_micDeviceBox);

    m_themeBox = new QComboBox(this);
    m_themeBox->addItems({"Light", "Dark"});
    form->addRow("Theme", m_themeBox);

    m_languageBox = new QComboBox(this);
    m_languageBox->addItems({"English", "বাংলা"});
    form->addRow("Language", m_languageBox);

    m_aiModelBox = new QComboBox(this);
    m_aiModelBox->addItems({"llama3.2", "mistral", "gpt-4o-mini"});
    form->addRow("AI model", m_aiModelBox);

    m_difficultyBox = new QComboBox(this);
    m_difficultyBox->addItems({"Easy", "Medium", "Hard"});
    form->addRow("Default difficulty", m_difficultyBox);

    m_notificationsCheck = new QCheckBox("Enable notifications", this);
    form->addRow("Notifications", m_notificationsCheck);

    rootLayout->addLayout(form);

    m_savedLabel = new QLabel(this);
    m_savedLabel->setStyleSheet("color: #2a7a2a;");
    rootLayout->addWidget(m_savedLabel);

    auto *buttonRow = new QHBoxLayout();
    auto *saveBtn = new QPushButton("Save", this);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsScreen::onSaveClicked);
    buttonRow->addWidget(saveBtn);

    auto *backBtn = new QPushButton("Back to Dashboard", this);
    connect(backBtn, &QPushButton::clicked, this, &SettingsScreen::backRequested);
    buttonRow->addWidget(backBtn);

    rootLayout->addLayout(buttonRow);
    rootLayout->addStretch();

    populateDeviceLists();
    loadFromSettings();
}

void SettingsScreen::populateDeviceLists()
{
    m_cameraDeviceBox->clear();
    m_cameraDeviceBox->addItem("System default", QString());
    for (const QCameraDevice &device : QMediaDevices::videoInputs()) {
        m_cameraDeviceBox->addItem(device.description(), QString::fromUtf8(device.id()));
    }

    m_micDeviceBox->clear();
    m_micDeviceBox->addItem("System default", QString());
    for (const QAudioDevice &device : QMediaDevices::audioInputs()) {
        m_micDeviceBox->addItem(device.description(), QString::fromUtf8(device.id()));
    }
}

void SettingsScreen::loadFromSettings()
{
    const QString cameraId = m_settings->cameraDevice();
    const int cameraIndex = cameraId.isEmpty() ? 0 : m_cameraDeviceBox->findData(cameraId);
    m_cameraDeviceBox->setCurrentIndex(cameraIndex >= 0 ? cameraIndex : 0);

    const QString micId = m_settings->microphoneDevice();
    const int micIndex = micId.isEmpty() ? 0 : m_micDeviceBox->findData(micId);
    m_micDeviceBox->setCurrentIndex(micIndex >= 0 ? micIndex : 0);

    m_themeBox->setCurrentText(m_settings->theme());
    m_languageBox->setCurrentText(m_settings->language());
    m_aiModelBox->setCurrentText(m_settings->aiModel());
    m_difficultyBox->setCurrentText(m_settings->defaultDifficulty());
    m_notificationsCheck->setChecked(m_settings->notificationsEnabled());
}

void SettingsScreen::onSaveClicked()
{
    m_settings->setCameraDevice(m_cameraDeviceBox->currentData().toString());
    m_settings->setMicrophoneDevice(m_micDeviceBox->currentData().toString());
    m_settings->setTheme(m_themeBox->currentText());
    m_settings->setLanguage(m_languageBox->currentText());
    m_settings->setAiModel(m_aiModelBox->currentText());
    m_settings->setDefaultDifficulty(m_difficultyBox->currentText());
    m_settings->setNotificationsEnabled(m_notificationsCheck->isChecked());

    // Apply the theme change immediately, not just on next launch.
    ThemeManager::applyTheme(m_themeBox->currentText());

    m_savedLabel->setText("Settings saved.");
}

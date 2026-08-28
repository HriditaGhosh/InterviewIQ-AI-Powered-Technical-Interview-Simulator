#include "ProfileScreen.h"
#include "DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFont>
#include <QCryptographicHash>

ProfileScreen::ProfileScreen(DatabaseManager *db, int userId, QWidget *parent)
    : QWidget(parent), m_db(db), m_userId(userId)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout();
    auto *backBtn = new QPushButton("← Back", this);
    connect(backBtn, &QPushButton::clicked, this, &ProfileScreen::backRequested);
    header->addWidget(backBtn);

    auto *title = new QLabel("Profile", this);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    header->addWidget(title);
    header->addStretch();
    rootLayout->addLayout(header);

    auto *infoForm = new QFormLayout();
    m_nameLabel = new QLabel(this);
    m_emailLabel = new QLabel(this);
    infoForm->addRow("Name", m_nameLabel);
    infoForm->addRow("Email", m_emailLabel);
    rootLayout->addLayout(infoForm);

    auto *changePwTitle = new QLabel("Change Password", this);
    changePwTitle->setFont(QFont("Arial", 14, QFont::Bold));
    rootLayout->addWidget(changePwTitle);

    auto *pwForm = new QFormLayout();
    m_currentPassword = new QLineEdit(this);
    m_currentPassword->setEchoMode(QLineEdit::Password);
    m_newPassword = new QLineEdit(this);
    m_newPassword->setEchoMode(QLineEdit::Password);
    m_confirmPassword = new QLineEdit(this);
    m_confirmPassword->setEchoMode(QLineEdit::Password);
    pwForm->addRow("Current password", m_currentPassword);
    pwForm->addRow("New password", m_newPassword);
    pwForm->addRow("Confirm new password", m_confirmPassword);
    rootLayout->addLayout(pwForm);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    rootLayout->addWidget(m_statusLabel);

    auto *changeBtn = new QPushButton("Update Password", this);
    connect(changeBtn, &QPushButton::clicked, this, &ProfileScreen::onChangePasswordClicked);
    rootLayout->addWidget(changeBtn);
    connect(m_confirmPassword, &QLineEdit::returnPressed, changeBtn, &QPushButton::click);

    rootLayout->addStretch();

    refresh();
}

void ProfileScreen::refresh()
{
    const UserProfile profile = m_db->fetchUserProfile(m_userId);
    m_nameLabel->setText(profile.name);
    m_emailLabel->setText(profile.email);

    m_currentPassword->clear();
    m_newPassword->clear();
    m_confirmPassword->clear();
    m_statusLabel->clear();
}

void ProfileScreen::onChangePasswordClicked()
{
    const QString current = m_currentPassword->text();
    const QString newPassword = m_newPassword->text();
    const QString confirm = m_confirmPassword->text();

    if (current.isEmpty() || newPassword.isEmpty()) {
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText("Fill in your current password and a new password.");
        return;
    }
    if (newPassword.length() < 6) {
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText("New password must be at least 6 characters.");
        return;
    }
    if (newPassword != confirm) {
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText("New passwords don't match.");
        return;
    }

    const QByteArray currentHash =
        QCryptographicHash::hash(current.toUtf8(), QCryptographicHash::Sha256).toHex();
    if (!m_db->verifyPasswordById(m_userId, QString::fromUtf8(currentHash))) {
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText("Current password is incorrect.");
        return;
    }

    const QByteArray newHash =
        QCryptographicHash::hash(newPassword.toUtf8(), QCryptographicHash::Sha256).toHex();
    if (!m_db->updatePasswordById(m_userId, QString::fromUtf8(newHash))) {
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setText("Could not update the password — please try again.");
        return;
    }

    m_currentPassword->clear();
    m_newPassword->clear();
    m_confirmPassword->clear();
    m_statusLabel->setStyleSheet("color: #2a7a2a;");
    m_statusLabel->setText("Password updated.");
}

#pragma once

#include <QWidget>

class DatabaseManager;
class QLabel;
class QLineEdit;
class QPushButton;

/**
 * Profile / account screen (spec module 1: profile management, password
 * change). Shows the logged-in user's name/email (read-only — changing
 * those isn't wired up, only the password) and a change-password form:
 * current password, new password, confirm.
 */
class ProfileScreen : public QWidget {
    Q_OBJECT
public:
    explicit ProfileScreen(DatabaseManager *db, int userId, QWidget *parent = nullptr);

    // Re-reads name/email from the DB and clears the password fields.
    // Call this whenever the screen becomes visible.
    void refresh();

signals:
    void backRequested();

private slots:
    void onChangePasswordClicked();

private:
    DatabaseManager *m_db;
    int m_userId;

    QLabel *m_nameLabel = nullptr;
    QLabel *m_emailLabel = nullptr;

    QLineEdit *m_currentPassword = nullptr;
    QLineEdit *m_newPassword = nullptr;
    QLineEdit *m_confirmPassword = nullptr;
    QLabel *m_statusLabel = nullptr;
};

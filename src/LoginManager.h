#pragma once

#include <QWidget>

class DatabaseManager;
class QLineEdit;
class QLabel;
class QComboBox;
class QStackedWidget;

/**
 * Handles Registration, Login, and Forgot Password UI in a small internal
 * QStackedWidget (login form / register form / reset-password form).
 *
 * Password reset uses a security question chosen + answered at
 * registration, since this is a fully offline desktop app with no email
 * service to send a reset link through.
 */
class LoginManager : public QWidget {
    Q_OBJECT
public:
    explicit LoginManager(DatabaseManager *db, QWidget *parent = nullptr);

signals:
    void loginSucceeded(int userId, const QString &displayName);

public slots:
    void attemptLogin(const QString &email, const QString &password);
    void registerUser(const QString &name, const QString &email, const QString &password,
                       const QString &securityQuestion, const QString &securityAnswer);

    // Step 1 of reset: look up the account's security question by email.
    void beginPasswordReset(const QString &email);
    // Step 2 of reset: verify the answer and, if correct, set a new password.
    void completePasswordReset(const QString &email, const QString &securityAnswer,
                                const QString &newPassword, const QString &confirmPassword);

private:
    QWidget *buildLoginForm();
    QWidget *buildRegisterForm();
    QWidget *buildResetForm();
    void showResetStep(int step); // 0 = enter email, 1 = answer question

    DatabaseManager *m_db;
    QStackedWidget *m_stack = nullptr;

    QLineEdit *m_loginEmail = nullptr;
    QLineEdit *m_loginPassword = nullptr;
    QLabel *m_loginError = nullptr;

    QLineEdit *m_registerName = nullptr;
    QLineEdit *m_registerEmail = nullptr;
    QLineEdit *m_registerPassword = nullptr;
    QComboBox *m_registerSecurityQuestion = nullptr;
    QLineEdit *m_registerSecurityAnswer = nullptr;
    QLabel *m_registerError = nullptr;

    QWidget *m_resetStepEmail = nullptr;
    QWidget *m_resetStepAnswer = nullptr;
    QLineEdit *m_resetEmail = nullptr;
    QLabel *m_resetError = nullptr;
    QLabel *m_resetQuestionLabel = nullptr;
    QLineEdit *m_resetAnswer = nullptr;
    QLineEdit *m_resetNewPassword = nullptr;
    QLineEdit *m_resetConfirmPassword = nullptr;
    QLabel *m_resetAnswerError = nullptr;
};

#include "LoginManager.h"
#include "DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QFont>
#include <QCryptographicHash>

LoginManager::LoginManager(DatabaseManager *db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->addStretch();

    auto *centerRow = new QHBoxLayout();
    centerRow->addStretch();

    m_stack = new QStackedWidget(this);
    m_stack->setFixedWidth(360);
    m_stack->addWidget(buildLoginForm());   // index 0
    m_stack->addWidget(buildRegisterForm()); // index 1
    m_stack->addWidget(buildResetForm());    // index 2
    centerRow->addWidget(m_stack);

    centerRow->addStretch();
    rootLayout->addLayout(centerRow);
    rootLayout->addStretch();
}

QWidget *LoginManager::buildLoginForm()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("InterviewIQ Login", page);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *form = new QFormLayout();
    m_loginEmail = new QLineEdit(page);
    m_loginEmail->setPlaceholderText("you@example.com");
    m_loginPassword = new QLineEdit(page);
    m_loginPassword->setEchoMode(QLineEdit::Password);
    form->addRow("Email", m_loginEmail);
    form->addRow("Password", m_loginPassword);
    layout->addLayout(form);

    m_loginError = new QLabel(page);
    m_loginError->setStyleSheet("color: red;");
    m_loginError->setWordWrap(true);
    layout->addWidget(m_loginError);

    auto *loginBtn = new QPushButton("Log In", page);
    connect(loginBtn, &QPushButton::clicked, this, [this]() {
        attemptLogin(m_loginEmail->text().trimmed(), m_loginPassword->text());
    });
    layout->addWidget(loginBtn);

    // Pressing Enter in the password field also submits.
    connect(m_loginPassword, &QLineEdit::returnPressed, loginBtn, &QPushButton::click);

    auto *switchRow = new QHBoxLayout();
    auto *switchLabel = new QLabel("Don't have an account?", page);
    auto *switchBtn = new QPushButton("Register", page);
    switchBtn->setFlat(true);
    connect(switchBtn, &QPushButton::clicked, this, [this]() { m_stack->setCurrentIndex(1); });
    switchRow->addWidget(switchLabel);
    switchRow->addWidget(switchBtn);
    switchRow->addStretch();
    layout->addLayout(switchRow);

    auto *forgotBtn = new QPushButton("Forgot password?", page);
    forgotBtn->setFlat(true);
    connect(forgotBtn, &QPushButton::clicked, this, [this]() {
        m_resetEmail->setText(m_loginEmail->text().trimmed());
        showResetStep(0);
        m_stack->setCurrentIndex(2);
    });
    layout->addWidget(forgotBtn);

    layout->addStretch();
    return page;
}

QWidget *LoginManager::buildRegisterForm()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("Create Account", page);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *form = new QFormLayout();
    m_registerName = new QLineEdit(page);
    m_registerEmail = new QLineEdit(page);
    m_registerPassword = new QLineEdit(page);
    m_registerPassword->setEchoMode(QLineEdit::Password);
    form->addRow("Name", m_registerName);
    form->addRow("Email", m_registerEmail);
    form->addRow("Password", m_registerPassword);

    m_registerSecurityQuestion = new QComboBox(page);
    m_registerSecurityQuestion->addItems({
        "What was your first pet's name?",
        "What city were you born in?",
        "What is your favorite teacher's name?",
        "What was the model of your first phone?",
        "What is your childhood best friend's name?",
    });
    m_registerSecurityAnswer = new QLineEdit(page);
    m_registerSecurityAnswer->setPlaceholderText("Used to recover your account — no email needed");
    form->addRow("Security question", m_registerSecurityQuestion);
    form->addRow("Answer", m_registerSecurityAnswer);

    layout->addLayout(form);

    m_registerError = new QLabel(page);
    m_registerError->setStyleSheet("color: red;");
    m_registerError->setWordWrap(true);
    layout->addWidget(m_registerError);

    auto *registerBtn = new QPushButton("Create Account", page);
    connect(registerBtn, &QPushButton::clicked, this, [this]() {
        registerUser(m_registerName->text().trimmed(), m_registerEmail->text().trimmed(),
                     m_registerPassword->text(), m_registerSecurityQuestion->currentText(),
                     m_registerSecurityAnswer->text());
    });
    layout->addWidget(registerBtn);
    connect(m_registerSecurityAnswer, &QLineEdit::returnPressed, registerBtn, &QPushButton::click);

    auto *switchRow = new QHBoxLayout();
    auto *switchLabel = new QLabel("Already have an account?", page);
    auto *switchBtn = new QPushButton("Log In", page);
    switchBtn->setFlat(true);
    connect(switchBtn, &QPushButton::clicked, this, [this]() { m_stack->setCurrentIndex(0); });
    switchRow->addWidget(switchLabel);
    switchRow->addWidget(switchBtn);
    switchRow->addStretch();
    layout->addLayout(switchRow);

    layout->addStretch();
    return page;
}

QWidget *LoginManager::buildResetForm()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("Reset Password", page);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // --- Step 0: enter email, look up the security question -------------
    m_resetStepEmail = new QWidget(page);
    auto *emailStepLayout = new QVBoxLayout(m_resetStepEmail);
    emailStepLayout->setContentsMargins(0, 0, 0, 0);

    auto *emailForm = new QFormLayout();
    m_resetEmail = new QLineEdit(m_resetStepEmail);
    m_resetEmail->setPlaceholderText("you@example.com");
    emailForm->addRow("Email", m_resetEmail);
    emailStepLayout->addLayout(emailForm);

    m_resetError = new QLabel(m_resetStepEmail);
    m_resetError->setStyleSheet("color: red;");
    m_resetError->setWordWrap(true);
    emailStepLayout->addWidget(m_resetError);

    auto *continueBtn = new QPushButton("Continue", m_resetStepEmail);
    connect(continueBtn, &QPushButton::clicked, this, [this]() {
        beginPasswordReset(m_resetEmail->text().trimmed());
    });
    emailStepLayout->addWidget(continueBtn);
    connect(m_resetEmail, &QLineEdit::returnPressed, continueBtn, &QPushButton::click);

    layout->addWidget(m_resetStepEmail);

    // --- Step 1: answer the security question, set a new password -------
    m_resetStepAnswer = new QWidget(page);
    auto *answerStepLayout = new QVBoxLayout(m_resetStepAnswer);
    answerStepLayout->setContentsMargins(0, 0, 0, 0);

    m_resetQuestionLabel = new QLabel(m_resetStepAnswer);
    m_resetQuestionLabel->setWordWrap(true);
    m_resetQuestionLabel->setFont(QFont("Arial", 11, QFont::Bold));
    answerStepLayout->addWidget(m_resetQuestionLabel);

    auto *answerForm = new QFormLayout();
    m_resetAnswer = new QLineEdit(m_resetStepAnswer);
    m_resetNewPassword = new QLineEdit(m_resetStepAnswer);
    m_resetNewPassword->setEchoMode(QLineEdit::Password);
    m_resetConfirmPassword = new QLineEdit(m_resetStepAnswer);
    m_resetConfirmPassword->setEchoMode(QLineEdit::Password);
    answerForm->addRow("Your answer", m_resetAnswer);
    answerForm->addRow("New password", m_resetNewPassword);
    answerForm->addRow("Confirm password", m_resetConfirmPassword);
    answerStepLayout->addLayout(answerForm);

    m_resetAnswerError = new QLabel(m_resetStepAnswer);
    m_resetAnswerError->setStyleSheet("color: red;");
    m_resetAnswerError->setWordWrap(true);
    answerStepLayout->addWidget(m_resetAnswerError);

    auto *resetBtn = new QPushButton("Reset Password", m_resetStepAnswer);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        completePasswordReset(m_resetEmail->text().trimmed(), m_resetAnswer->text(),
                               m_resetNewPassword->text(), m_resetConfirmPassword->text());
    });
    answerStepLayout->addWidget(resetBtn);
    connect(m_resetConfirmPassword, &QLineEdit::returnPressed, resetBtn, &QPushButton::click);

    layout->addWidget(m_resetStepAnswer);

    auto *backBtn = new QPushButton("Back to Login", page);
    backBtn->setFlat(true);
    connect(backBtn, &QPushButton::clicked, this, [this]() { m_stack->setCurrentIndex(0); });
    layout->addWidget(backBtn);

    layout->addStretch();
    showResetStep(0);
    return page;
}

void LoginManager::showResetStep(int step)
{
    m_resetStepEmail->setVisible(step == 0);
    m_resetStepAnswer->setVisible(step == 1);
}

void LoginManager::attemptLogin(const QString &email, const QString &password)
{
    if (email.isEmpty() || password.isEmpty()) {
        m_loginError->setStyleSheet("color: red;");
        m_loginError->setText("Please enter both email and password.");
        return;
    }

    const QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
    int userId = -1;
    if (m_db->verifyLogin(email, QString::fromUtf8(hash), userId)) {
        m_loginError->clear();
        emit loginSucceeded(userId, email);
    } else {
        m_loginError->setStyleSheet("color: red;");
        m_loginError->setText("Invalid email or password.");
    }
}

void LoginManager::registerUser(const QString &name, const QString &email, const QString &password,
                                 const QString &securityQuestion, const QString &securityAnswer)
{
    if (name.isEmpty() || email.isEmpty() || password.isEmpty()) {
        m_registerError->setText("Please fill in all fields.");
        return;
    }
    if (password.length() < 6) {
        m_registerError->setText("Password must be at least 6 characters.");
        return;
    }
    if (securityAnswer.trimmed().isEmpty()) {
        m_registerError->setText("Please answer the security question — it's how you'll reset your password.");
        return;
    }

    const QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();

    // Normalize the answer (trim + lowercase) before hashing so "Fluffy" and
    // "fluffy " both verify correctly later.
    const QString normalizedAnswer = securityAnswer.trimmed().toLower();
    const QByteArray answerHash =
        QCryptographicHash::hash(normalizedAnswer.toUtf8(), QCryptographicHash::Sha256).toHex();

    const int userId = m_db->createUser(name, email, QString::fromUtf8(hash),
                                         securityQuestion, QString::fromUtf8(answerHash));
    if (userId > 0) {
        m_registerError->clear();
        emit loginSucceeded(userId, name);
    } else if (userId == -2) {
        m_registerError->setText("An account with this email already exists — try logging in instead.");
    } else {
        m_registerError->setText("Could not create account due to a database error. Please try again.");
    }
}

void LoginManager::beginPasswordReset(const QString &email)
{
    if (email.isEmpty()) {
        m_resetError->setText("Enter the email you registered with.");
        return;
    }

    const QString question = m_db->fetchSecurityQuestion(email);
    if (question.isEmpty()) {
        m_resetError->setText("No account found with that email.");
        return;
    }

    m_resetError->clear();
    m_resetQuestionLabel->setText(question);
    m_resetAnswer->clear();
    m_resetNewPassword->clear();
    m_resetConfirmPassword->clear();
    m_resetAnswerError->clear();
    showResetStep(1);
}

void LoginManager::completePasswordReset(const QString &email, const QString &securityAnswer,
                                          const QString &newPassword, const QString &confirmPassword)
{
    if (newPassword.length() < 6) {
        m_resetAnswerError->setText("New password must be at least 6 characters.");
        return;
    }
    if (newPassword != confirmPassword) {
        m_resetAnswerError->setText("Passwords don't match.");
        return;
    }

    const QString normalizedAnswer = securityAnswer.trimmed().toLower();
    const QByteArray answerHash =
        QCryptographicHash::hash(normalizedAnswer.toUtf8(), QCryptographicHash::Sha256).toHex();

    if (!m_db->verifySecurityAnswer(email, QString::fromUtf8(answerHash))) {
        m_resetAnswerError->setText("That answer doesn't match our records.");
        return;
    }

    const QByteArray newHash =
        QCryptographicHash::hash(newPassword.toUtf8(), QCryptographicHash::Sha256).toHex();
    if (!m_db->updatePassword(email, QString::fromUtf8(newHash))) {
        m_resetAnswerError->setText("Could not update the password — please try again.");
        return;
    }

    m_loginEmail->setText(email);
    m_loginPassword->clear();
    m_loginError->setText("Password reset. Log in with your new password.");
    m_loginError->setStyleSheet("color: #2a7a2a;");
    m_stack->setCurrentIndex(0);
}

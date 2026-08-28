#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>
#include <QMessageBox>
#include <QIcon>

#include "DatabaseManager.h"
#include "LoginManager.h"
#include "DashboardWidget.h"
#include "HistoryManager.h"
#include "InterviewScreen.h"
#include "InterviewSetupDialog.h"
#include "SettingsManager.h"
#include "SettingsScreen.h"
#include "ThemeManager.h"
#include "AchievementsScreen.h"
#include "LeaderboardScreen.h"
#include "CodingRoundScreen.h"
#include "ProfileScreen.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("InterviewIQ");
    QApplication::setOrganizationName("InterviewIQ");
    QApplication::setWindowIcon(QIcon(":/icons/app_icon.svg")); // from resources/resources.qrc

    SettingsManager settings;
    ThemeManager::applyTheme(settings.theme()); // apply the saved theme (Light/Dark) at launch

    DatabaseManager db;
    if (!db.open("interviewiq.db")) {
        QMessageBox::warning(nullptr, "Database Error",
                              "Failed to open the local database. The app will still run, "
                              "but nothing will be saved.");
    }

    QMainWindow window;
    window.setWindowTitle("InterviewIQ — AI Powered Technical Interview Simulator");
    window.resize(1100, 750);

    auto *stack = new QStackedWidget(&window);
    window.setCentralWidget(stack);

    // --- Login screen ------------------------------------------------
    auto *loginManager = new LoginManager(&db, stack);
    stack->addWidget(loginManager);

    // Settings and the Coding Round practice tool are reachable before
    // login too (neither is user-scoped / persisted per-user), so they're
    // built once up front rather than lazily like the post-login screens.
    auto *settingsScreen = new SettingsScreen(&settings, stack);
    stack->addWidget(settingsScreen);

    auto *codingRoundScreen = new CodingRoundScreen(stack);
    stack->addWidget(codingRoundScreen);

    // The dashboard, history, interview, achievements, and leaderboard
    // screens are created lazily once we know the logged-in user, since
    // they're scoped to a user id.
    DashboardWidget *dashboard = nullptr;
    HistoryManager *history = nullptr;
    InterviewScreen *interviewScreen = nullptr;
    AchievementsScreen *achievementsScreen = nullptr;
    LeaderboardScreen *leaderboardScreen = nullptr;
    ProfileScreen *profileScreen = nullptr;

    QObject::connect(settingsScreen, &SettingsScreen::backRequested, &window, [&]() {
        stack->setCurrentWidget(dashboard ? static_cast<QWidget *>(dashboard) : static_cast<QWidget *>(loginManager));
    });

    QObject::connect(codingRoundScreen, &CodingRoundScreen::backRequested, &window, [&]() {
        stack->setCurrentWidget(dashboard ? static_cast<QWidget *>(dashboard) : static_cast<QWidget *>(loginManager));
    });

    QObject::connect(loginManager, &LoginManager::loginSucceeded, &window,
        [&](int userId, const QString & /*displayName*/) {

        // Build the post-login screens now that we have a user id.
        dashboard = new DashboardWidget(&db, userId, stack);
        history = new HistoryManager(&db, userId, stack);
        interviewScreen = new InterviewScreen(&db, &settings, stack);
        achievementsScreen = new AchievementsScreen(&db, userId, stack);
        leaderboardScreen = new LeaderboardScreen(&db, userId, stack);
        profileScreen = new ProfileScreen(&db, userId, stack);

        stack->addWidget(dashboard);
        stack->addWidget(history);
        stack->addWidget(interviewScreen);
        stack->addWidget(achievementsScreen);
        stack->addWidget(leaderboardScreen);
        stack->addWidget(profileScreen);

        QObject::connect(dashboard, &DashboardWidget::startInterviewRequested, &window, [&, userId]() {
            InterviewSetupDialog dialog(&window);
            if (dialog.exec() == QDialog::Accepted) {
                stack->setCurrentWidget(interviewScreen);
                interviewScreen->beginInterview(
                    userId, dialog.selectedCategory(), dialog.selectedDifficulty(),
                    dialog.selectedDurationMinutes());
            }
        });

        QObject::connect(dashboard, &DashboardWidget::historyRequested, &window, [&]() {
            history->refresh();
            stack->setCurrentWidget(history);
        });

        QObject::connect(dashboard, &DashboardWidget::settingsRequested, &window, [&]() {
            stack->setCurrentWidget(settingsScreen);
        });

        QObject::connect(dashboard, &DashboardWidget::codingRoundRequested, &window, [&]() {
            stack->setCurrentWidget(codingRoundScreen);
        });

        QObject::connect(dashboard, &DashboardWidget::achievementsRequested, &window, [&]() {
            achievementsScreen->refresh();
            stack->setCurrentWidget(achievementsScreen);
        });

        QObject::connect(dashboard, &DashboardWidget::leaderboardRequested, &window, [&]() {
            leaderboardScreen->refresh();
            stack->setCurrentWidget(leaderboardScreen);
        });

        QObject::connect(dashboard, &DashboardWidget::profileRequested, &window, [&]() {
            profileScreen->refresh();
            stack->setCurrentWidget(profileScreen);
        });

        QObject::connect(history, &HistoryManager::backRequested, &window, [&]() {
            stack->setCurrentWidget(dashboard);
        });

        QObject::connect(achievementsScreen, &AchievementsScreen::backRequested, &window, [&]() {
            stack->setCurrentWidget(dashboard);
        });

        QObject::connect(leaderboardScreen, &LeaderboardScreen::backRequested, &window, [&]() {
            stack->setCurrentWidget(dashboard);
        });

        QObject::connect(profileScreen, &ProfileScreen::backRequested, &window, [&]() {
            stack->setCurrentWidget(dashboard);
        });

        QObject::connect(interviewScreen, &InterviewScreen::interviewCompleted, &window,
            [&](int /*interviewId*/, const QJsonObject & /*report*/) {
                dashboard->refresh();
                stack->setCurrentWidget(dashboard);
            });

        dashboard->refresh();
        stack->setCurrentWidget(dashboard);
    });

    stack->setCurrentWidget(loginManager);
    window.show();
    return app.exec();
}

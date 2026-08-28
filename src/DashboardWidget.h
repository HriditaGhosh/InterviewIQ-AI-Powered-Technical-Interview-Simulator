#pragma once

#include <QWidget>

class DatabaseManager;
class QLabel;
class QPushButton;
class QVBoxLayout;

/**
 * Displays: Total Interviews, Average Score, Strong/Weak Subjects,
 * Daily Streak, Practice Hours, Last Interview Result, Upcoming Practice Goal.
 */
class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(DatabaseManager *db, int userId, QWidget *parent = nullptr);

public slots:
    void refresh();

signals:
    void startInterviewRequested();
    void historyRequested();
    void settingsRequested();
    void achievementsRequested();
    void leaderboardRequested();
    void codingRoundRequested();
    void profileRequested();

private:
    QWidget *buildStatCard(const QString &icon, const QString &title, QLabel **valueLabelOut);

    DatabaseManager *m_db;
    int m_userId;

    QLabel *m_totalInterviewsValue = nullptr;
    QLabel *m_averageScoreValue = nullptr;
    QLabel *m_strongSubjectValue = nullptr;
    QLabel *m_weakSubjectValue = nullptr;
    QLabel *m_streakValue = nullptr;
    QLabel *m_practiceHoursValue = nullptr;
    QLabel *m_lastResultValue = nullptr;
    QLabel *m_goalValue = nullptr;

    QVBoxLayout *m_chartContainerLayout = nullptr;
};

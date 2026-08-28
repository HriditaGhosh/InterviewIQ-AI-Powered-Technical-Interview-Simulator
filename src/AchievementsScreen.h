#pragma once

#include <QWidget>

class DatabaseManager;

/**
 * Shows the full badge catalog (see Achievements.h), with unlocked badges
 * highlighted and locked ones greyed out. Badges are unlocked by
 * InterviewManager::finishInterview() as scores/totals cross thresholds.
 */
class AchievementsScreen : public QWidget {
    Q_OBJECT
public:
    explicit AchievementsScreen(DatabaseManager *db, int userId, QWidget *parent = nullptr);

    // Re-reads unlocked badges from the DB and redraws. Call this whenever
    // the screen becomes visible, since new badges may have unlocked since
    // it was built.
    void refresh();

signals:
    void backRequested();

private:
    DatabaseManager *m_db;
    int m_userId;
    QWidget *m_cardContainer = nullptr;
};

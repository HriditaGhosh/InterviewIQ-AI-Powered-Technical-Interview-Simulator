#pragma once

#include <QWidget>

class DatabaseManager;
class QTableWidget;

/**
 * Ranks all users by average overall interview score (DatabaseManager::
 * fetchLeaderboard). The current user's row is highlighted so they can
 * find themselves in a longer list.
 */
class LeaderboardScreen : public QWidget {
    Q_OBJECT
public:
    explicit LeaderboardScreen(DatabaseManager *db, int currentUserId, QWidget *parent = nullptr);

    void refresh();

signals:
    void backRequested();

private:
    DatabaseManager *m_db;
    int m_currentUserId;
    QTableWidget *m_table = nullptr;
};

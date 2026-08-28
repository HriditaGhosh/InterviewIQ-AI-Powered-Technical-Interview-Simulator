#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <QPair>
#include <QStringList>

struct DashboardStats {
    int totalInterviews = 0;
    double averageScore = 0.0;
    QString strongSubject;
    QString weakSubject;
    int dailyStreak = 0;
    double practiceHours = 0.0;
    double lastInterviewScore = -1.0;
    QString lastInterviewCategory;
};

struct HistoryRow {
    int interviewId = 0;
    QString date;
    QString category;
    int durationMin = 0;
    double overallScore = 0.0;
    QString feedback;
};

struct LeaderboardEntry {
    int userId = 0;
    QString name;
    double averageScore = 0.0;
    int totalInterviews = 0;
    double practiceHours = 0.0;
};

struct UserProfile {
    QString name;
    QString email;
};

/**
 * Owns the SQLite connection and exposes CRUD helpers used across the app.
 * Uses prepared statements everywhere to avoid SQL injection and to match
 * the "prepared statements" concept called out in the project spec.
 */
class DatabaseManager {
public:
    DatabaseManager() = default;
    ~DatabaseManager();

    // Opens (creating if necessary) the SQLite file at `path` and applies
    // database/schema.sql if the tables don't exist yet.
    bool open(const QString &path);
    void close();

    // --- Users -------------------------------------------------------
    // Returns the new user id, or -1 on failure (e.g. duplicate email).
    // securityAnswerHash should already be normalized (trimmed/lowercased)
    // and hashed by the caller, same as passwordHash.
    int createUser(const QString &name, const QString &email, const QString &passwordHash,
                   const QString &securityQuestion, const QString &securityAnswerHash);
    bool verifyLogin(const QString &email, const QString &passwordHash, int &userIdOut);

    // --- Password reset (offline app: security question instead of email) --
    // Returns the stored question for `email`, or an empty string if no
    // account with that email exists.
    QString fetchSecurityQuestion(const QString &email);
    bool verifySecurityAnswer(const QString &email, const QString &answerHash);
    bool updatePassword(const QString &email, const QString &newPasswordHash);

    // --- Profile / change password (while already logged in) -------------
    UserProfile fetchUserProfile(int userId);
    bool verifyPasswordById(int userId, const QString &passwordHash);
    bool updatePasswordById(int userId, const QString &newPasswordHash);

    // --- Interviews ----------------------------------------------------
    int createInterview(int userId, const QString &category, const QString &difficulty, int durationMin);
    bool saveResults(int interviewId, double technical, double communication,
                      double confidence, double eyeContactPct, double speakingWpm, double overall);
    bool saveHistoryEntry(int interviewId, const QString &resultSummary, const QString &feedback);

    // --- Dashboard -----------------------------------------------------
    DashboardStats fetchDashboardStats(int userId);

    // Per-category average score, e.g. for weak-topic recommendation and
    // the category bar chart. Sorted ascending by score (weakest first).
    QVector<QPair<QString, double>> fetchCategoryAverages(int userId);

    // (date label, overall_score) pairs ordered oldest -> newest, for the
    // progress line chart.
    QVector<QPair<QString, double>> fetchScoreProgress(int userId, int limit = 20);

    // --- History ---------------------------------------------------------
    QVector<HistoryRow> fetchHistory(int userId, const QString &categoryFilter = QString());

    // --- Achievements ----------------------------------------------------
    bool unlockAchievement(int userId, const QString &badgeCode);
    QStringList unlockedAchievements(int userId);

    // --- Leaderboard -------------------------------------------------
    // Ranked by each user's average overall_score across all their
    // interviews (descending). Only users with at least one completed
    // interview are included.
    QVector<LeaderboardEntry> fetchLeaderboard(int limit = 50);

private:
    QSqlDatabase m_db;
    bool ensureSchema();
};

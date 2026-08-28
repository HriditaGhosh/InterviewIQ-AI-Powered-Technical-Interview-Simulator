#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDate>
#include <algorithm>

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open(const QString &path)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "interviewiq_connection");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qWarning() << "DatabaseManager: failed to open" << path << m_db.lastError().text();
        return false;
    }

    return ensureSchema();
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::ensureSchema()
{
    // Loads database/schema.sql relative to the executable and executes each
    // statement. This lets the same schema.sql file serve both `sqlite3 <
    // schema.sql` (manual setup) and first-run auto-provisioning here.
    QFile schemaFile("database/schema.sql");
    if (!schemaFile.exists()) {
        // Fall back to a path relative to the project source tree during dev.
        schemaFile.setFileName("../database/schema.sql");
    }
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "DatabaseManager: could not locate schema.sql; "
                       "assuming schema already exists.";
        return true;
    }

    const QString sql = QTextStream(&schemaFile).readAll();
    QSqlQuery query(m_db);
    for (const QString &statement : sql.split(';', Qt::SkipEmptyParts)) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty()) continue;
        if (!query.exec(trimmed)) {
            qWarning() << "DatabaseManager: schema statement failed:" << query.lastError().text();
        }
    }
    return true;
}

// NOTE: every query below uses positional "?" placeholders with
// addBindValue() (bound strictly in left-to-right order), rather than named
// ":placeholder" ones. Named placeholders hit a "Parameter count mismatch"
// driver error on some Qt/SQLite builds (seen on Qt 6.11 MinGW); positional
// binding sidesteps that whole class of bug since there's no placeholder
// name parsing involved.

int DatabaseManager::createUser(const QString &name, const QString &email, const QString &passwordHash,
                                 const QString &securityQuestion, const QString &securityAnswerHash)
{
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Users (name, email, password_hash, security_question, security_answer_hash) "
        "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(email);
    query.addBindValue(passwordHash);
    query.addBindValue(securityQuestion);
    query.addBindValue(securityAnswerHash);

    if (!query.exec()) {
        qWarning() << "createUser failed:" << query.lastError().text();
        // -2 = genuinely a duplicate email (UNIQUE constraint), so the
        // caller can show an accurate message instead of assuming
        // "already registered" for every possible failure (bad schema,
        // locked file, etc. would previously get the same misleading text).
        return query.lastError().text().contains("UNIQUE", Qt::CaseInsensitive) ? -2 : -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::verifyLogin(const QString &email, const QString &passwordHash, int &userIdOut)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM Users WHERE email = ? AND password_hash = ?");
    query.addBindValue(email);
    query.addBindValue(passwordHash);

    if (!query.exec() || !query.next()) {
        return false;
    }
    userIdOut = query.value(0).toInt();
    return true;
}

QString DatabaseManager::fetchSecurityQuestion(const QString &email)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT security_question FROM Users WHERE email = ?");
    query.addBindValue(email);

    if (!query.exec() || !query.next()) {
        return QString();
    }
    return query.value(0).toString();
}

bool DatabaseManager::verifySecurityAnswer(const QString &email, const QString &answerHash)
{
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id FROM Users WHERE email = ? AND security_answer_hash = ?");
    query.addBindValue(email);
    query.addBindValue(answerHash);

    return query.exec() && query.next();
}

bool DatabaseManager::updatePassword(const QString &email, const QString &newPasswordHash)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE Users SET password_hash = ? WHERE email = ?");
    query.addBindValue(newPasswordHash);
    query.addBindValue(email);

    if (!query.exec()) {
        qWarning() << "updatePassword failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

UserProfile DatabaseManager::fetchUserProfile(int userId)
{
    UserProfile profile;

    QSqlQuery query(m_db);
    query.prepare("SELECT name, email FROM Users WHERE id = ?");
    query.addBindValue(userId);

    if (!query.exec() || !query.next()) {
        return profile;
    }
    profile.name = query.value(0).toString();
    profile.email = query.value(1).toString();
    return profile;
}

bool DatabaseManager::verifyPasswordById(int userId, const QString &passwordHash)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM Users WHERE id = ? AND password_hash = ?");
    query.addBindValue(userId);
    query.addBindValue(passwordHash);

    return query.exec() && query.next();
}

bool DatabaseManager::updatePasswordById(int userId, const QString &newPasswordHash)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE Users SET password_hash = ? WHERE id = ?");
    query.addBindValue(newPasswordHash);
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "updatePasswordById failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

int DatabaseManager::createInterview(int userId, const QString &category, const QString &difficulty, int durationMin)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Interview (user_id, category, difficulty, duration_min) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(category);
    query.addBindValue(difficulty);
    query.addBindValue(durationMin);

    if (!query.exec()) {
        qWarning() << "createInterview failed:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::saveResults(int interviewId, double technical, double communication,
                                   double confidence, double eyeContactPct, double speakingWpm, double overall)
{
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Results (interview_id, technical_score, communication_score, "
        "confidence_score, eye_contact_pct, speaking_wpm, overall_score) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(interviewId);
    query.addBindValue(technical);
    query.addBindValue(communication);
    query.addBindValue(confidence);
    query.addBindValue(eyeContactPct);
    query.addBindValue(speakingWpm);
    query.addBindValue(overall);

    if (!query.exec()) {
        qWarning() << "saveResults failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::saveHistoryEntry(int interviewId, const QString &resultSummary, const QString &feedback)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO History (interview_id, result_summary, feedback) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(interviewId);
    query.addBindValue(resultSummary);
    query.addBindValue(feedback);

    if (!query.exec()) {
        qWarning() << "saveHistoryEntry failed:" << query.lastError().text();
        return false;
    }
    return true;
}

DashboardStats DatabaseManager::fetchDashboardStats(int userId)
{
    DashboardStats stats;

    // Total interviews + average score
    {
        QSqlQuery query(m_db);
        query.prepare(
            "SELECT COUNT(*), COALESCE(AVG(r.overall_score), 0) "
            "FROM Interview i JOIN Results r ON r.interview_id = i.id "
            "WHERE i.user_id = ?");
        query.addBindValue(userId);
        if (query.exec() && query.next()) {
            stats.totalInterviews = query.value(0).toInt();
            stats.averageScore = query.value(1).toDouble();
        }
    }

    // Practice hours (sum of interview durations)
    {
        QSqlQuery query(m_db);
        query.prepare("SELECT COALESCE(SUM(duration_min), 0) FROM Interview WHERE user_id = ?");
        query.addBindValue(userId);
        if (query.exec() && query.next()) {
            stats.practiceHours = query.value(0).toDouble() / 60.0;
        }
    }

    // Strong/weak subject: category with highest / lowest average score
    {
        const auto categoryAverages = fetchCategoryAverages(userId);
        if (!categoryAverages.isEmpty()) {
            stats.weakSubject = categoryAverages.first().first;    // ascending -> weakest first
            stats.strongSubject = categoryAverages.last().first;   // strongest last
        }
    }

    // Last interview result
    {
        QSqlQuery query(m_db);
        query.prepare(
            "SELECT i.category, r.overall_score FROM Interview i "
            "JOIN Results r ON r.interview_id = i.id "
            "WHERE i.user_id = ? ORDER BY i.date DESC LIMIT 1");
        query.addBindValue(userId);
        if (query.exec() && query.next()) {
            stats.lastInterviewCategory = query.value(0).toString();
            stats.lastInterviewScore = query.value(1).toDouble();
        }
    }

    // Daily streak: count consecutive calendar days (ending today or
    // yesterday) that have at least one interview.
    {
        QSqlQuery query(m_db);
        query.prepare(
            "SELECT DISTINCT date(date) FROM Interview WHERE user_id = ? ORDER BY date(date) DESC");
        query.addBindValue(userId);
        if (query.exec()) {
            QVector<QDate> days;
            while (query.next()) {
                days.append(QDate::fromString(query.value(0).toString(), Qt::ISODate));
            }
            int streak = 0;
            QDate expected = QDate::currentDate();
            for (const QDate &d : days) {
                if (d == expected) {
                    ++streak;
                    expected = expected.addDays(-1);
                } else if (streak == 0 && d == expected.addDays(-1)) {
                    // Allow the streak to start "yesterday" if no interview today yet.
                    ++streak;
                    expected = d.addDays(-1);
                } else {
                    break;
                }
            }
            stats.dailyStreak = streak;
        }
    }

    return stats;
}

QVector<QPair<QString, double>> DatabaseManager::fetchCategoryAverages(int userId)
{
    QVector<QPair<QString, double>> results;

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT i.category, AVG(r.overall_score) AS avg_score "
        "FROM Interview i JOIN Results r ON r.interview_id = i.id "
        "WHERE i.user_id = ? "
        "GROUP BY i.category "
        "ORDER BY avg_score ASC");
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "fetchCategoryAverages failed:" << query.lastError().text();
        return results;
    }
    while (query.next()) {
        results.append({query.value(0).toString(), query.value(1).toDouble()});
    }
    return results;
}

QVector<QPair<QString, double>> DatabaseManager::fetchScoreProgress(int userId, int limit)
{
    QVector<QPair<QString, double>> results;

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT i.date, r.overall_score FROM Interview i "
        "JOIN Results r ON r.interview_id = i.id "
        "WHERE i.user_id = ? "
        "ORDER BY i.date DESC LIMIT ?");
    query.addBindValue(userId);
    query.addBindValue(limit);

    if (!query.exec()) {
        qWarning() << "fetchScoreProgress failed:" << query.lastError().text();
        return results;
    }
    while (query.next()) {
        results.append({query.value(0).toString(), query.value(1).toDouble()});
    }
    std::reverse(results.begin(), results.end()); // oldest -> newest for the chart
    return results;
}

QVector<HistoryRow> DatabaseManager::fetchHistory(int userId, const QString &categoryFilter)
{
    QVector<HistoryRow> rows;

    QString sql =
        "SELECT i.id, i.date, i.category, i.duration_min, "
        "COALESCE(r.overall_score, 0), COALESCE(h.feedback, '') "
        "FROM Interview i "
        "LEFT JOIN Results r ON r.interview_id = i.id "
        "LEFT JOIN History h ON h.interview_id = i.id "
        "WHERE i.user_id = ?";
    if (!categoryFilter.isEmpty()) {
        sql += " AND i.category = ?";
    }
    sql += " ORDER BY i.date DESC";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.addBindValue(userId);
    if (!categoryFilter.isEmpty()) {
        query.addBindValue(categoryFilter);
    }

    if (!query.exec()) {
        qWarning() << "fetchHistory failed:" << query.lastError().text();
        return rows;
    }

    while (query.next()) {
        HistoryRow row;
        row.interviewId = query.value(0).toInt();
        row.date = query.value(1).toString();
        row.category = query.value(2).toString();
        row.durationMin = query.value(3).toInt();
        row.overallScore = query.value(4).toDouble();
        row.feedback = query.value(5).toString();
        rows.append(row);
    }
    return rows;
}

bool DatabaseManager::unlockAchievement(int userId, const QString &badgeCode)
{
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT OR IGNORE INTO Achievements (user_id, badge_code) VALUES (?, ?)");
    query.addBindValue(userId);
    query.addBindValue(badgeCode);

    if (!query.exec()) {
        qWarning() << "unlockAchievement failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

QStringList DatabaseManager::unlockedAchievements(int userId)
{
    QStringList badges;

    QSqlQuery query(m_db);
    query.prepare("SELECT badge_code FROM Achievements WHERE user_id = ? ORDER BY unlocked_date");
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "unlockedAchievements failed:" << query.lastError().text();
        return badges;
    }
    while (query.next()) {
        badges << query.value(0).toString();
    }
    return badges;
}

QVector<LeaderboardEntry> DatabaseManager::fetchLeaderboard(int limit)
{
    QVector<LeaderboardEntry> entries;

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT u.id, u.name, "
        "       AVG(r.overall_score) AS avg_score, "
        "       COUNT(DISTINCT i.id) AS total_interviews, "
        "       COALESCE(SUM(i.duration_min), 0) / 60.0 AS practice_hours "
        "FROM Users u "
        "JOIN Interview i ON i.user_id = u.id "
        "JOIN Results r ON r.interview_id = i.id "
        "GROUP BY u.id "
        "ORDER BY avg_score DESC "
        "LIMIT ?");
    query.addBindValue(limit);

    if (!query.exec()) {
        qWarning() << "fetchLeaderboard failed:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        LeaderboardEntry entry;
        entry.userId = query.value(0).toInt();
        entry.name = query.value(1).toString();
        entry.averageScore = query.value(2).toDouble();
        entry.totalInterviews = query.value(3).toInt();
        entry.practiceHours = query.value(4).toDouble();
        entries.append(entry);
    }
    return entries;
}

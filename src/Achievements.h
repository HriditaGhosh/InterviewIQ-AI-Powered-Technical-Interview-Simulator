#pragma once

#include <QString>
#include <QVector>

/**
 * The badge catalog from the spec's Achievement System (module 19):
 * First Interview, 10 Interviews, Excellent Communication, DSA Expert,
 * Interview Master. This is the single source of truth for badge
 * code/title/description, used both by InterviewManager (to decide when
 * to call DatabaseManager::unlockAchievement) and AchievementsScreen (to
 * render the full catalog with locked/unlocked state).
 */
struct AchievementDef {
    QString code;
    QString title;
    QString description;
};

namespace Achievements {

inline const QVector<AchievementDef> &all()
{
    static const QVector<AchievementDef> kAll = {
        {"FIRST_INTERVIEW", "First Interview", "Complete your first mock interview."},
        {"TEN_INTERVIEWS", "10 Interviews", "Complete 10 mock interviews."},
        {"EXCELLENT_COMMUNICATION", "Excellent Communication", "Score 90+ on communication in a single interview."},
        {"DSA_EXPERT", "DSA Expert", "Score 90+ on a DSA interview."},
        {"INTERVIEW_MASTER", "Interview Master", "Score 90+ overall in a single interview."},
    };
    return kAll;
}

} // namespace Achievements

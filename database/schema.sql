-- InterviewIQ SQLite schema
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS Users (
    id                      INTEGER PRIMARY KEY AUTOINCREMENT,
    name                    TEXT NOT NULL,
    email                   TEXT NOT NULL UNIQUE,
    password_hash           TEXT NOT NULL,
    security_question       TEXT NOT NULL DEFAULT '',
    security_answer_hash    TEXT NOT NULL DEFAULT '',
    created_date            TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS Interview (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    category        TEXT NOT NULL,   -- DSA, OOP, DBMS, OS, CN, SE, HR, Mixed
    difficulty      TEXT NOT NULL,   -- Easy, Medium, Hard
    duration_min    INTEGER NOT NULL,
    date            TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS Results (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    interview_id        INTEGER NOT NULL REFERENCES Interview(id) ON DELETE CASCADE,
    technical_score     REAL NOT NULL DEFAULT 0,
    communication_score REAL NOT NULL DEFAULT 0,
    confidence_score    REAL NOT NULL DEFAULT 0,
    eye_contact_pct     REAL NOT NULL DEFAULT 0,
    speaking_wpm        REAL NOT NULL DEFAULT 0,
    overall_score       REAL NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS History (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    interview_id    INTEGER NOT NULL REFERENCES Interview(id) ON DELETE CASCADE,
    result_summary  TEXT,
    feedback        TEXT
);

CREATE TABLE IF NOT EXISTS Achievements (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    badge_code      TEXT NOT NULL,   -- e.g. FIRST_INTERVIEW, TEN_INTERVIEWS
    unlocked_date   TEXT NOT NULL DEFAULT (datetime('now')),
    UNIQUE(user_id, badge_code)
);

CREATE INDEX IF NOT EXISTS idx_interview_user   ON Interview(user_id);
CREATE INDEX IF NOT EXISTS idx_results_interview ON Results(interview_id);
CREATE INDEX IF NOT EXISTS idx_history_interview ON History(interview_id);

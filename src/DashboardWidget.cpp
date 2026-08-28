#include "DashboardWidget.h"
#include "DatabaseManager.h"
#include "ChartManager.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFont>

DashboardWidget::DashboardWidget(DatabaseManager *db, int userId, QWidget *parent)
    : QWidget(parent), m_db(db), m_userId(userId)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel("Dashboard", this);
    title->setFont(QFont("Arial", 20, QFont::Bold));
    header->addWidget(title);
    header->addStretch();

    auto *startBtn = new QPushButton("Start Interview", this);
    startBtn->setObjectName("startInterviewBtn");
    auto *codingBtn = new QPushButton("Coding Round", this);
    auto *historyBtn = new QPushButton("History", this);
    auto *achievementsBtn = new QPushButton("Achievements", this);
    auto *leaderboardBtn = new QPushButton("Leaderboard", this);
    auto *settingsBtn = new QPushButton("Settings", this);
    auto *profileBtn = new QPushButton("Profile", this);
    connect(startBtn, &QPushButton::clicked, this, &DashboardWidget::startInterviewRequested);
    connect(codingBtn, &QPushButton::clicked, this, &DashboardWidget::codingRoundRequested);
    connect(historyBtn, &QPushButton::clicked, this, &DashboardWidget::historyRequested);
    connect(achievementsBtn, &QPushButton::clicked, this, &DashboardWidget::achievementsRequested);
    connect(leaderboardBtn, &QPushButton::clicked, this, &DashboardWidget::leaderboardRequested);
    connect(settingsBtn, &QPushButton::clicked, this, &DashboardWidget::settingsRequested);
    connect(profileBtn, &QPushButton::clicked, this, &DashboardWidget::profileRequested);
    header->addWidget(profileBtn);
    header->addWidget(historyBtn);
    header->addWidget(achievementsBtn);
    header->addWidget(leaderboardBtn);
    header->addWidget(settingsBtn);
    header->addWidget(codingBtn);
    header->addWidget(startBtn);
    rootLayout->addLayout(header);

    // Stat card grid: 4 columns x 2 rows for the 8 dashboard stats.
    auto *grid = new QGridLayout();
    grid->setSpacing(12);
    grid->addWidget(buildStatCard("📊", "Total Interviews", &m_totalInterviewsValue), 0, 0);
    grid->addWidget(buildStatCard("📈", "Average Score", &m_averageScoreValue), 0, 1);
    grid->addWidget(buildStatCard("🏆", "Strong Subject", &m_strongSubjectValue), 0, 2);
    grid->addWidget(buildStatCard("📚", "Weak Subject", &m_weakSubjectValue), 0, 3);
    grid->addWidget(buildStatCard("🔥", "Daily Streak", &m_streakValue), 1, 0);
    grid->addWidget(buildStatCard("⏱️", "Practice Hours", &m_practiceHoursValue), 1, 1);
    grid->addWidget(buildStatCard("📝", "Last Interview", &m_lastResultValue), 1, 2);
    grid->addWidget(buildStatCard("🎯", "Upcoming Goal", &m_goalValue), 1, 3);
    rootLayout->addLayout(grid);

    auto *chartSectionLabel = new QLabel("Progress Trend", this);
    chartSectionLabel->setFont(QFont("Arial", 14, QFont::Bold));
    rootLayout->addWidget(chartSectionLabel);

    m_chartContainerLayout = new QVBoxLayout();
    rootLayout->addLayout(m_chartContainerLayout);
    rootLayout->addStretch();

    refresh();
}

QWidget *DashboardWidget::buildStatCard(const QString &icon, const QString &title, QLabel **valueLabelOut)
{
    auto *card = new QFrame(this);
    card->setObjectName("statCard");
    card->setFrameShape(QFrame::StyledPanel);
    card->setMinimumSize(180, 96);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(QString("%1  %2").arg(icon, title), card);
    titleLabel->setObjectName("statCardTitle");
    titleLabel->setFont(QFont("Arial", 10, QFont::DemiBold));

    auto *valueLabel = new QLabel("—", card);
    valueLabel->setObjectName("statCardValue");
    valueLabel->setFont(QFont("Arial", 19, QFont::Bold));

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addStretch();

    *valueLabelOut = valueLabel;
    return card;
}

void DashboardWidget::refresh()
{
    const DashboardStats stats = m_db->fetchDashboardStats(m_userId);

    m_totalInterviewsValue->setText(QString::number(stats.totalInterviews));
    m_averageScoreValue->setText(QString("%1").arg(stats.averageScore, 0, 'f', 1));
    m_strongSubjectValue->setText(stats.strongSubject.isEmpty() ? "—" : stats.strongSubject);
    m_weakSubjectValue->setText(stats.weakSubject.isEmpty() ? "—" : stats.weakSubject);
    m_streakValue->setText(QString("%1 day%2").arg(stats.dailyStreak).arg(stats.dailyStreak == 1 ? "" : "s"));
    m_practiceHoursValue->setText(QString("%1 hrs").arg(stats.practiceHours, 0, 'f', 1));

    if (stats.lastInterviewScore >= 0) {
        m_lastResultValue->setText(QString("%1 (%2)").arg(stats.lastInterviewScore, 0, 'f', 0).arg(stats.lastInterviewCategory));
    } else {
        m_lastResultValue->setText("No interviews yet");
    }

    // Simple heuristic "upcoming goal": beat the average score by 5 points
    // in the weakest subject, or "Take your first interview" if there's no
    // history yet.
    if (stats.totalInterviews == 0) {
        m_goalValue->setText("Take your first interview");
    } else {
        m_goalValue->setText(QString("Score %1+ in %2")
                                  .arg(int(stats.averageScore) + 5)
                                  .arg(stats.weakSubject.isEmpty() ? "any category" : stats.weakSubject));
    }

    // Clear and rebuild the progress chart.
    QLayoutItem *child;
    while ((child = m_chartContainerLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    const auto progress = m_db->fetchScoreProgress(m_userId);
    if (!progress.isEmpty()) {
        m_chartContainerLayout->addWidget(ChartManager::buildProgressLineChart(progress, this));
    } else {
        m_chartContainerLayout->addWidget(new QLabel("No score history yet — complete an interview to see your trend.", this));
    }
}

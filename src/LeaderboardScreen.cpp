#include "LeaderboardScreen.h"
#include "DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QColor>

LeaderboardScreen::LeaderboardScreen(DatabaseManager *db, int currentUserId, QWidget *parent)
    : QWidget(parent), m_db(db), m_currentUserId(currentUserId)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout();
    auto *backBtn = new QPushButton("← Back", this);
    connect(backBtn, &QPushButton::clicked, this, &LeaderboardScreen::backRequested);
    header->addWidget(backBtn);

    auto *title = new QLabel("Leaderboard", this);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    header->addWidget(title);
    header->addStretch();
    rootLayout->addLayout(header);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Rank", "Name", "Average Score", "Interviews", "Practice Hours"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(m_table);

    refresh();
}

void LeaderboardScreen::refresh()
{
    const auto entries = m_db->fetchLeaderboard();
    m_table->setRowCount(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
        const auto &entry = entries.at(i);
        const bool isCurrentUser = (entry.userId == m_currentUserId);

        auto *rankItem = new QTableWidgetItem(QString::number(i + 1));
        auto *nameItem = new QTableWidgetItem(isCurrentUser ? entry.name + " (you)" : entry.name);
        auto *scoreItem = new QTableWidgetItem(QString::number(entry.averageScore, 'f', 1));
        auto *countItem = new QTableWidgetItem(QString::number(entry.totalInterviews));
        auto *hoursItem = new QTableWidgetItem(QString::number(entry.practiceHours, 'f', 1));

        if (isCurrentUser) {
            for (auto *item : {rankItem, nameItem, scoreItem, countItem, hoursItem}) {
                item->setBackground(QColor("#3a6ea5"));
                item->setForeground(QColor("#ffffff"));
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
        }

        m_table->setItem(i, 0, rankItem);
        m_table->setItem(i, 1, nameItem);
        m_table->setItem(i, 2, scoreItem);
        m_table->setItem(i, 3, countItem);
        m_table->setItem(i, 4, hoursItem);
    }
}

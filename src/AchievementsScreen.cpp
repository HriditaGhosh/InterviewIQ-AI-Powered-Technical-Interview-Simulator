#include "AchievementsScreen.h"
#include "DatabaseManager.h"
#include "Achievements.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QFont>

AchievementsScreen::AchievementsScreen(DatabaseManager *db, int userId, QWidget *parent)
    : QWidget(parent), m_db(db), m_userId(userId)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel("Achievements", this);
    title->setFont(QFont("Arial", 20, QFont::Bold));
    header->addWidget(title);
    header->addStretch();

    auto *backBtn = new QPushButton("Back to Dashboard", this);
    connect(backBtn, &QPushButton::clicked, this, &AchievementsScreen::backRequested);
    header->addWidget(backBtn);
    rootLayout->addLayout(header);

    m_cardContainer = new QWidget(this);
    rootLayout->addWidget(m_cardContainer);
    rootLayout->addStretch();

    refresh();
}

void AchievementsScreen::refresh()
{
    qDeleteAll(m_cardContainer->findChildren<QFrame *>(QString(), Qt::FindDirectChildrenOnly));
    delete m_cardContainer->layout();
    // Rebuild the grid from scratch each refresh — badge count is small
    // (5 in the current catalog) so this is cheap.
    auto *grid = new QGridLayout(m_cardContainer);

    const QStringList unlocked = m_db->unlockedAchievements(m_userId);
    const auto &catalog = Achievements::all();

    int row = 0, col = 0;
    constexpr int kColumns = 3;
    for (const auto &def : catalog) {
        const bool isUnlocked = unlocked.contains(def.code);

        auto *card = new QFrame(m_cardContainer);
        card->setFrameShape(QFrame::StyledPanel);
        card->setMinimumSize(220, 120);
        card->setStyleSheet(isUnlocked
            ? "QFrame { border: 2px solid #d4af37; border-radius: 8px; }"
            : "QFrame { border: 2px solid gray; border-radius: 8px; }");

        auto *cardLayout = new QVBoxLayout(card);

        auto *badgeIcon = new QLabel(isUnlocked ? "🏆" : "🔒", card);
        badgeIcon->setFont(QFont("Arial", 28));
        badgeIcon->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(badgeIcon);

        auto *titleLabel = new QLabel(def.title, card);
        titleLabel->setFont(QFont("Arial", 12, QFont::Bold));
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setWordWrap(true);
        if (!isUnlocked) titleLabel->setStyleSheet("color: gray;");
        cardLayout->addWidget(titleLabel);

        auto *descLabel = new QLabel(def.description, card);
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet(isUnlocked ? "" : "color: gray;");
        cardLayout->addWidget(descLabel);

        grid->addWidget(card, row, col);
        if (++col >= kColumns) { col = 0; ++row; }
    }
}

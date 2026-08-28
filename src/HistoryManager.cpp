#include "HistoryManager.h"
#include "DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFont>

HistoryManager::HistoryManager(DatabaseManager *db, int userId, QWidget *parent)
    : QWidget(parent), m_db(db), m_userId(userId)
{
    auto *rootLayout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout();
    auto *backBtn = new QPushButton("← Back", this);
    connect(backBtn, &QPushButton::clicked, this, &HistoryManager::backRequested);
    header->addWidget(backBtn);

    auto *title = new QLabel("Interview History", this);
    title->setFont(QFont("Arial", 18, QFont::Bold));
    header->addWidget(title);
    header->addStretch();

    m_categoryFilterBox = new QComboBox(this);
    m_categoryFilterBox->addItem("All Categories", "");
    for (const QString &cat : {"Data Structures", "Algorithms", "OOP", "DBMS",
                                "Operating System", "Computer Networks",
                                "Software Engineering", "HR Interview", "Mixed"}) {
        m_categoryFilterBox->addItem(cat, cat);
    }
    connect(m_categoryFilterBox, &QComboBox::currentTextChanged, this, [this](const QString &) {
        filterByCategory(m_categoryFilterBox->currentData().toString());
    });
    header->addWidget(m_categoryFilterBox);
    rootLayout->addLayout(header);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Date", "Category", "Duration (min)", "Score", "Feedback", ""});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(m_table);

    refresh();
}

void HistoryManager::refresh()
{
    populateTable(m_db->fetchHistory(m_userId, m_activeFilter));
}

void HistoryManager::filterByCategory(const QString &category)
{
    m_activeFilter = category;
    refresh();
}

void HistoryManager::populateTable(const QVector<HistoryRow> &rows)
{
    m_table->setRowCount(rows.size());

    for (int i = 0; i < rows.size(); ++i) {
        const HistoryRow &row = rows.at(i);
        m_table->setItem(i, 0, new QTableWidgetItem(row.date));
        m_table->setItem(i, 1, new QTableWidgetItem(row.category));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(row.durationMin)));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(row.overallScore, 'f', 1)));
        m_table->setItem(i, 4, new QTableWidgetItem(row.feedback));

        auto *viewBtn = new QPushButton("View Report", m_table);
        const int interviewId = row.interviewId;
        connect(viewBtn, &QPushButton::clicked, this, [this, interviewId]() {
            emit reportRequested(interviewId);
        });
        m_table->setCellWidget(i, 5, viewBtn);
    }
}

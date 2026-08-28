#pragma once

#include <QWidget>
#include <QVector>

#include "DatabaseManager.h" // for the HistoryRow struct

class QTableWidget;
class QComboBox;

/**
 * Lists past interviews (Date, Time, Category, Duration, Score, Feedback)
 * pulled from the History/Interview/Results tables, with a category filter
 * and a "view report" action per row.
 */
class HistoryManager : public QWidget {
    Q_OBJECT
public:
    explicit HistoryManager(DatabaseManager *db, int userId, QWidget *parent = nullptr);

public slots:
    void refresh();
    void filterByCategory(const QString &category);

signals:
    void reportRequested(int interviewId);
    void backRequested();

private:
    void populateTable(const QVector<HistoryRow> &rows);

    DatabaseManager *m_db;
    int m_userId;
    QString m_activeFilter;

    QTableWidget *m_table = nullptr;
    QComboBox *m_categoryFilterBox = nullptr;
};

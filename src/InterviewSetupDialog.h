#pragma once

#include <QDialog>

class QComboBox;
class QSpinBox;

/**
 * Small modal shown before starting an interview: pick category,
 * difficulty, and duration. Matches the spec's "Interview Categories" and
 * "Timer" modules.
 */
class InterviewSetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit InterviewSetupDialog(QWidget *parent = nullptr);

    QString selectedCategory() const;
    QString selectedDifficulty() const;
    int selectedDurationMinutes() const;

private:
    QComboBox *m_categoryBox = nullptr;
    QComboBox *m_difficultyBox = nullptr;
    QComboBox *m_durationBox = nullptr;
};

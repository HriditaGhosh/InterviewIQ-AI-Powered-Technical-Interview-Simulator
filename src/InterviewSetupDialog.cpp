#include "InterviewSetupDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFont>

InterviewSetupDialog::InterviewSetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Start New Interview");
    setMinimumWidth(320);

    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel("Interview Setup", this);
    title->setFont(QFont("Arial", 14, QFont::Bold));
    layout->addWidget(title);

    auto *form = new QFormLayout();

    m_categoryBox = new QComboBox(this);
    m_categoryBox->addItems({
        "Data Structures", "Algorithms", "OOP", "DBMS",
        "Operating System", "Computer Networks",
        "Software Engineering", "HR Interview", "Mixed"
    });
    form->addRow("Category", m_categoryBox);

    m_difficultyBox = new QComboBox(this);
    m_difficultyBox->addItems({"Easy", "Medium", "Hard"});
    m_difficultyBox->setCurrentText("Medium");
    form->addRow("Difficulty", m_difficultyBox);

    m_durationBox = new QComboBox(this);
    m_durationBox->addItems({"15", "30", "45", "60"});
    m_durationBox->setCurrentText("30");
    form->addRow("Duration (minutes)", m_durationBox);

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QString InterviewSetupDialog::selectedCategory() const { return m_categoryBox->currentText(); }
QString InterviewSetupDialog::selectedDifficulty() const { return m_difficultyBox->currentText(); }
int InterviewSetupDialog::selectedDurationMinutes() const { return m_durationBox->currentText().toInt(); }

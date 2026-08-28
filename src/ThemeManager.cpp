#include "ThemeManager.h"

#include <QApplication>

namespace ThemeManager {

QString lightStyleSheet()
{
    // Mostly falls back to the platform's native Qt style, but the stat
    // cards and the primary "Start Interview" action still need explicit
    // styling so they stand out against the native white background.
    return QStringLiteral(R"(
        QFrame#statCard {
            background-color: #f4f5f7;
            border: 1px solid #e1e3e8;
            border-radius: 8px;
        }
        QPushButton#startInterviewBtn {
            background-color: #2fa84f;
            color: #ffffff;
            font-weight: bold;
            border: none;
            border-radius: 6px;
            padding: 7px 18px;
        }
        QPushButton#startInterviewBtn:hover {
            background-color: #38bb59;
        }
        QPushButton#startInterviewBtn:pressed {
            background-color: #268f42;
        }
    )");
}

QString darkStyleSheet()
{
    return QStringLiteral(R"(
        QWidget {
            background-color: #1e1f22;
            color: #e6e6e6;
        }
        QMainWindow, QDialog {
            background-color: #1e1f22;
        }
        QLabel {
            color: #e6e6e6;
            background-color: transparent;
        }
        QLineEdit, QTextEdit, QComboBox, QSpinBox {
            background-color: #2b2d30;
            color: #e6e6e6;
            border: 1px solid #3c3f41;
            border-radius: 4px;
            padding: 4px;
        }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
            border: 1px solid #5c9eff;
        }
        QComboBox QAbstractItemView {
            background-color: #2b2d30;
            color: #e6e6e6;
            selection-background-color: #3a6ea5;
        }
        QPushButton {
            background-color: #3a6ea5;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #4c85c2;
        }
        QPushButton:pressed {
            background-color: #2f5a86;
        }
        QPushButton:disabled {
            background-color: #3c3f41;
            color: #8a8a8a;
        }
        QPushButton:flat {
            background-color: transparent;
            color: #5c9eff;
        }
        /* Primary call-to-action: visually distinct from every other
           dashboard button so it reads as the main action at a glance. */
        QPushButton#startInterviewBtn {
            background-color: #2fa84f;
            color: #ffffff;
            font-weight: bold;
            border: none;
            border-radius: 6px;
            padding: 7px 18px;
        }
        QPushButton#startInterviewBtn:hover {
            background-color: #38c25c;
        }
        QPushButton#startInterviewBtn:pressed {
            background-color: #24893f;
        }
        /* Stat cards: a step lighter than the #1e1f22 window background so
           each card reads as a distinct surface instead of blending in. */
        QFrame#statCard {
            background-color: #2b2d30;
            border: 1px solid #3c3f41;
            border-radius: 8px;
        }
        QFrame#statCard QLabel#statCardTitle {
            color: #9a9ea6;
        }
        QFrame#statCard QLabel#statCardValue {
            color: #ffffff;
        }
        QProgressBar {
            background-color: #2b2d30;
            border: 1px solid #3c3f41;
            border-radius: 4px;
            text-align: center;
            color: #e6e6e6;
        }
        QProgressBar::chunk {
            background-color: #3a6ea5;
            border-radius: 4px;
        }
        QTableWidget, QTableView {
            background-color: #2b2d30;
            color: #e6e6e6;
            gridline-color: #3c3f41;
            selection-background-color: #3a6ea5;
            selection-color: #ffffff;
        }
        QHeaderView::section {
            background-color: #1e1f22;
            color: #e6e6e6;
            border: 1px solid #3c3f41;
            padding: 4px;
        }
        QCheckBox {
            color: #e6e6e6;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            background-color: #1e1f22;
        }
        QScrollBar::handle {
            background-color: #3c3f41;
            border-radius: 4px;
        }
        QStackedWidget {
            background-color: #1e1f22;
        }
    )");
}

void applyTheme(const QString &themeName)
{
    if (auto *app = qApp) {
        app->setStyleSheet(themeName == QLatin1String("Dark") ? darkStyleSheet() : lightStyleSheet());
    }
}

} // namespace ThemeManager

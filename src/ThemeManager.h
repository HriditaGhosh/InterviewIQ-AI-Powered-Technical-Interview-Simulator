#pragma once

#include <QString>

/**
 * Turns a theme name ("Light" | "Dark") into an actual applied
 * qApp->setStyleSheet(...) call. This is the missing link between
 * SettingsManager::theme() (which only persists the choice) and the UI
 * actually looking different.
 */
namespace ThemeManager {

// Applies the given theme to the whole QApplication immediately.
// Safe to call at startup and again any time the user changes it in Settings.
void applyTheme(const QString &themeName);

// The stylesheet strings themselves, exposed in case a widget needs to
// query the current theme name rather than just applying one.
QString lightStyleSheet();
QString darkStyleSheet();

} // namespace ThemeManager

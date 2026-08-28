cmake_minimum_required(VERSION 3.20)
project(InterviewIQ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Charts
    Multimedia
    MultimediaWidgets
    Sql
    Gui
    Svg
)

set(SOURCES
    src/main.cpp
    src/LoginManager.cpp
    src/DashboardWidget.cpp
    src/InterviewManager.cpp
    src/InterviewScreen.cpp
    src/InterviewSetupDialog.cpp
    src/DatabaseManager.cpp
    src/ChartManager.cpp
    src/PdfReportGenerator.cpp
    src/SettingsManager.cpp
    src/SettingsScreen.cpp
    src/ThemeManager.cpp
    src/HistoryManager.cpp
    src/CameraController.cpp
    src/AudioRecorder.cpp
    src/PythonBridge.cpp
    src/AchievementsScreen.cpp
    src/LeaderboardScreen.cpp
    src/CodingRoundScreen.cpp
    src/CodingJudge.cpp
    src/ProfileScreen.cpp
)

set(RESOURCES
    resources/resources.qrc
)

set(HEADERS
    src/LoginManager.h
    src/DashboardWidget.h
    src/InterviewManager.h
    src/InterviewScreen.h
    src/InterviewSetupDialog.h
    src/DatabaseManager.h
    src/ChartManager.h
    src/PdfReportGenerator.h
    src/SettingsManager.h
    src/SettingsScreen.h
    src/ThemeManager.h
    src/HistoryManager.h
    src/CameraController.h
    src/AudioRecorder.h
    src/PythonBridge.h
    src/QuestionBank.h
    src/AchievementsScreen.h
    src/LeaderboardScreen.h
    src/CodingRoundScreen.h
    src/CodingJudge.h
    src/CodingProblem.h
    src/Achievements.h
    src/ProfileScreen.h
)

add_executable(InterviewIQ WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(InterviewIQ PRIVATE
    Qt6::Widgets
    Qt6::Charts
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Sql
    Qt6::Gui
    Qt6::Svg
)

target_include_directories(InterviewIQ PRIVATE src)

# Copy the python/ directory next to the built binary so PythonBridge can
# find the AI scripts at runtime via a relative path.
add_custom_command(TARGET InterviewIQ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/python
        $<TARGET_FILE_DIR:InterviewIQ>/python
)

# Same deal for database/schema.sql — DatabaseManager::ensureSchema() looks
# for it via a relative path too, so without this copy the Users/Interview/
# etc. tables never get created (CREATE TABLE statements silently skipped),
# and every query against them fails.
add_custom_command(TARGET InterviewIQ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/database
        $<TARGET_FILE_DIR:InterviewIQ>/database
)

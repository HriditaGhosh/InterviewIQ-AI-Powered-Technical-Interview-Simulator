#include "CodingJudge.h"

#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QDir>

namespace CodingJudge {

namespace {
constexpr int kCompileTimeoutMs = 10000;
constexpr int kRunTimeoutMs = 5000;
}

CodingRunReport runSubmission(const QString &sourceCode, const QVector<CodingTestCase> &tests)
{
    CodingRunReport report;

    const QString compiler = QStandardPaths::findExecutable("g++");
    if (compiler.isEmpty()) {
        report.compileError =
            "Couldn't find g++ on PATH. Install MinGW-w64 (Windows) or your "
            "platform's g++ package, then make sure it's on PATH and restart the app.";
        return report;
    }

    QTemporaryDir workDir;
    if (!workDir.isValid()) {
        report.compileError = "Could not create a temporary build directory.";
        return report;
    }

    const QString sourcePath = QDir(workDir.path()).filePath("submission.cpp");
    const QString exePath = QDir(workDir.path()).filePath(
#ifdef Q_OS_WIN
        "submission.exe"
#else
        "submission"
#endif
    );

    QFile sourceFile(sourcePath);
    if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        report.compileError = "Could not write the submission to a temp file.";
        return report;
    }
    {
        QTextStream out(&sourceFile);
        out << sourceCode;
    }
    sourceFile.close();

    // --- Compile ----------------------------------------------------------
    QProcess compileProcess;
    compileProcess.start(compiler, {"-O2", "-std=c++17", "-o", exePath, sourcePath});
    if (!compileProcess.waitForStarted(3000) || !compileProcess.waitForFinished(kCompileTimeoutMs)) {
        report.compileError = "Compiler timed out or failed to start.";
        return report;
    }
    if (compileProcess.exitCode() != 0) {
        report.compileError = QString::fromUtf8(compileProcess.readAllStandardError());
        return report;
    }
    report.compileSucceeded = true;

    // --- Run each test case -------------------------------------------------
    QElapsedTimer totalTimer;
    totalTimer.start();

    for (const CodingTestCase &test : tests) {
        QProcess runProcess;
        QElapsedTimer testTimer;
        testTimer.start();

        runProcess.start(exePath, {});
        if (!runProcess.waitForStarted(2000)) {
            CodingTestResult result;
            result.actualOutput = "(failed to start submission binary)";
            report.results.append(result);
            continue;
        }

        runProcess.write(test.input.toUtf8());
        runProcess.closeWriteChannel();

        CodingTestResult result;
        if (!runProcess.waitForFinished(kRunTimeoutMs)) {
            runProcess.kill();
            runProcess.waitForFinished(1000);
            result.actualOutput = "(timed out)";
            result.elapsedMs = kRunTimeoutMs;
        } else {
            result.elapsedMs = testTimer.elapsed();
            result.actualOutput = QString::fromUtf8(runProcess.readAllStandardOutput());
        }

        result.passed = (result.actualOutput.trimmed() == test.expectedOutput.trimmed());
        report.results.append(result);
    }

    report.totalElapsedMs = totalTimer.elapsed();
    return report;
}

} // namespace CodingJudge

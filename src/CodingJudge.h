#pragma once

#include <QString>
#include <QVector>
#include "CodingProblem.h"

struct CodingTestResult {
    bool passed = false;
    QString actualOutput;
    qint64 elapsedMs = 0;
};

struct CodingRunReport {
    bool compileSucceeded = false;
    QString compileError;      // stderr from the compiler, if compileSucceeded is false
    QVector<CodingTestResult> results;
    qint64 totalElapsedMs = 0;
};

/**
 * Compiles a C++ submission with g++ and runs it against a set of test
 * cases, comparing trimmed stdout — the "compile/run, sample + hidden
 * test cases, execution time" part of spec module 12.
 *
 * Requires g++ (MinGW-w64 on Windows, or the system g++ on Linux/macOS)
 * to be on PATH; runSubmission() reports a clear compileError if it can't
 * find one, rather than crashing or hanging.
 *
 * This runs synchronously (blocking on QProcess::waitForFinished) since a
 * handful of small test cases compile/run in well under a second each —
 * simpler and more reliable than threading it for this scope. The caller
 * should show a "Running..." state and disable input while it's in
 * progress.
 */
namespace CodingJudge {

CodingRunReport runSubmission(const QString &sourceCode, const QVector<CodingTestCase> &tests);

} // namespace CodingJudge

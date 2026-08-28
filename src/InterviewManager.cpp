#include "InterviewManager.h"
#include "DatabaseManager.h"
#include "CameraController.h"
#include "PythonBridge.h"
#include "QuestionBank.h"
#include "Achievements.h"
#include "SettingsManager.h"

#include <QJsonArray>
#include <numeric>

namespace {
constexpr int kQuestionsPerInterview = 5;
}

InterviewManager::InterviewManager(DatabaseManager *db, SettingsManager *settings, QObject *parent)
    : QObject(parent), m_db(db), m_settings(settings)
{
    connect(&m_countdownTimer, &QTimer::timeout, this, &InterviewManager::onTick);
    m_countdownTimer.setInterval(1000);

    m_llmBridge = new PythonBridge("python/llm_feedback.py", this);
    connect(m_llmBridge, &PythonBridge::resultReceived, this, &InterviewManager::onLlmResult);
    connect(m_llmBridge, &PythonBridge::errorOccurred, this, [this](const QString &message) {
        // If llm_feedback.py's process itself dies (not just an exception
        // inside it, which already comes back as a normal {"error": ...}
        // JSON result), nothing would otherwise ever answer this question's
        // evaluation request and the UI would show "Evaluating..." forever.
        onLlmResult(QJsonObject{{"error", message}});
    });

    // Settings > AI model drives which backend/model llm_feedback.py uses —
    // "gpt-..." picks the OpenAI path, anything else is treated as an
    // Ollama model name (llama3.2, mistral, whatever the user has pulled).
    const QString aiModel = m_settings ? m_settings->aiModel() : QStringLiteral("llama3.2");
    if (aiModel.startsWith("gpt", Qt::CaseInsensitive)) {
        m_llmBridge->setEnvironmentVariable("AI_BACKEND", "openai");
        m_llmBridge->setEnvironmentVariable("OPENAI_MODEL", aiModel);
    } else {
        m_llmBridge->setEnvironmentVariable("AI_BACKEND", "ollama");
        m_llmBridge->setEnvironmentVariable("OLLAMA_MODEL", aiModel);
    }

    m_recommendationBridge = new PythonBridge("python/recommendation.py", this);
    connect(m_recommendationBridge, &PythonBridge::resultReceived, this, &InterviewManager::onRecommendationResult);
    connect(m_recommendationBridge, &PythonBridge::errorOccurred, this, [this](const QString &) {
        // Don't let a missing/broken recommendation.py hang the finish
        // flow forever — finish with no extra weak-topic suggestions.
        onRecommendationResult(QJsonObject{});
    });
}

void InterviewManager::configure(int userId, const QString &category, const QString &difficulty, int durationMinutes)
{
    m_userId = userId;
    m_category = category;
    m_difficulty = difficulty;
    m_durationSeconds = durationMinutes * 60;
    m_secondsElapsed = 0;
    m_results.clear();
    m_wpmSamples.clear();
}

void InterviewManager::start()
{
    setState(State::Initializing);

    m_interviewId = m_db->createInterview(m_userId, m_category, m_difficulty, m_durationSeconds / 60);

    m_llmBridge->start();
    m_recommendationBridge->start();
    if (m_camera) {
        m_camera->start();
    }

    m_questionQueue = QuestionBank::questionsFor(m_category, m_difficulty, kQuestionsPerInterview);
    m_currentQuestionIndex = -1;
    m_results.clear();

    m_countdownTimer.start();
    askNextQuestion();
}

void InterviewManager::askNextQuestion()
{
    ++m_currentQuestionIndex;
    if (m_currentQuestionIndex >= m_questionQueue.size()) {
        finishInterview();
        return;
    }
    setState(State::AskingQuestion);
    emit questionReady(m_questionQueue.at(m_currentQuestionIndex), m_currentQuestionIndex + 1, m_questionQueue.size());
    setState(State::AwaitingAnswer);
}

void InterviewManager::submitAnswerText(const QString &transcribedAnswer, double speakingWpm)
{
    if (m_state != State::AwaitingAnswer) return;

    setState(State::Evaluating);
    m_pendingAnswer = transcribedAnswer;
    if (speakingWpm > 0.0) m_wpmSamples.append(speakingWpm);

    const QString question = m_questionQueue.at(m_currentQuestionIndex);
    const QString reference = QuestionBank::referenceAnswerFor(question);

    m_llmBridge->sendRequest(QJsonObject{
        {"cmd", "evaluate"},
        {"question", question},
        {"answer", transcribedAnswer},
        {"reference_answer", reference},
    });
    // Execution continues in onLlmResult() once the Python side responds.
}

void InterviewManager::onLlmResult(const QJsonObject &result)
{
    // Only handle results while we're actually waiting on an evaluation —
    // guards against stray/late responses after stop().
    if (m_state != State::Evaluating) return;

    InterviewManager::QuestionResult qr;
    qr.question = m_questionQueue.at(m_currentQuestionIndex);
    qr.answerText = m_pendingAnswer;

    if (!result.contains("error")) {
        qr.accuracy = result.value("accuracy").toInt();
        qr.completeness = result.value("completeness").toInt();
        qr.clarity = result.value("clarity").toInt();
        qr.confidence = result.value("confidence").toInt();
        qr.summary = result.value("summary").toString();
    } else {
        qr.summary = "Evaluation unavailable: " + result.value("error").toString();
    }

    m_results.append(qr);
    emit evaluationReady(result);

    askNextQuestion();
}

void InterviewManager::finishInterview()
{
    setState(State::Evaluating);

    if (m_camera) {
        m_camera->stop();
    }
    m_countdownTimer.stop();

    // Average the per-question LLM scores into the four headline scores.
    double accSum = 0, compSum = 0, clarSum = 0, confSum = 0;
    for (const auto &r : m_results) {
        accSum += r.accuracy;
        compSum += r.completeness;
        clarSum += r.clarity;
        confSum += r.confidence;
    }
    const int n = qMax(1, m_results.size());
    const double technicalScore = ((accSum / n) + (compSum / n)) / 2.0 * 10.0;   // scale 1-10 -> 0-100
    const double communicationScore = (clarSum / n) * 10.0;
    double confidenceScore = (confSum / n) * 10.0;

    // Blend in camera-derived confidence signal if available: strong eye
    // contact nudges confidence up, "Nervous" as the dominant emotion nudges
    // it down.
    double eyeContactPct = 0.0;
    QString dominantEmotion = "Neutral";
    if (m_camera) {
        eyeContactPct = m_camera->eyeContactPercentage();
        dominantEmotion = m_camera->dominantEmotion();
        confidenceScore = (confidenceScore * 0.7) + (eyeContactPct * 0.3);
        if (dominantEmotion == "Nervous") confidenceScore *= 0.9;
    }

    const double overallScore = (technicalScore + communicationScore + confidenceScore) / 3.0;

    const double averageWpm = m_wpmSamples.isEmpty()
        ? 0.0
        : std::accumulate(m_wpmSamples.cbegin(), m_wpmSamples.cend(), 0.0) / m_wpmSamples.size();

    m_db->saveResults(m_interviewId, technicalScore, communicationScore, confidenceScore,
                       eyeContactPct, averageWpm, overallScore);

    const QString feedbackSummary = QString("Overall score: %1/100. Dominant emotion: %2.")
                                         .arg(overallScore, 0, 'f', 1)
                                         .arg(dominantEmotion);
    m_db->saveHistoryEntry(m_interviewId, feedbackSummary, feedbackSummary);

    // --- Achievements (spec module 19) -----------------------------------
    // Checked against this interview's own scores, plus the *updated*
    // dashboard totals (this interview is already saved above, so
    // totalInterviews already includes it).
    const DashboardStats updatedStats = m_db->fetchDashboardStats(m_userId);
    if (updatedStats.totalInterviews >= 1) m_db->unlockAchievement(m_userId, "FIRST_INTERVIEW");
    if (updatedStats.totalInterviews >= 10) m_db->unlockAchievement(m_userId, "TEN_INTERVIEWS");
    if (communicationScore >= 90.0) m_db->unlockAchievement(m_userId, "EXCELLENT_COMMUNICATION");
    if (m_category == "DSA" && technicalScore >= 90.0) m_db->unlockAchievement(m_userId, "DSA_EXPERT");
    if (overallScore >= 90.0) m_db->unlockAchievement(m_userId, "INTERVIEW_MASTER");

    // --- Weak-topic recommendation (spec module: personalized suggestions) --
    // Uses the *updated* per-category history (this interview's Results row
    // is already saved above) so recommendation.py sees it too. Finishing
    // continues in onRecommendationResult() once the Python side responds —
    // state stays Evaluating until then, matching the per-question flow.
    const auto categoryAverages = m_db->fetchCategoryAverages(m_userId);
    QJsonObject categoryScoresJson;
    for (const auto &pair : categoryAverages) categoryScoresJson[pair.first] = pair.second;
    m_recommendationBridge->sendRequest(QJsonObject{
        {"cmd", "recommend"},
        {"category_scores", categoryScoresJson},
    });
}

void InterviewManager::onRecommendationResult(const QJsonObject &result)
{
    if (m_state != State::Evaluating) return; // stray/late response after stop()

    QJsonArray extraSuggestions;
    for (const QJsonValue &value : result.value("suggestions").toArray()) {
        const QJsonObject suggestion = value.toObject();
        extraSuggestions.append(QString("Weak topic: %1 (score %2) — practice %3 at %4 difficulty")
                                     .arg(suggestion.value("topic").toString())
                                     .arg(suggestion.value("current_score").toDouble(), 0, 'f', 0)
                                     .arg(suggestion.value("focus").toString())
                                     .arg(suggestion.value("difficulty").toString()));
    }

    const QJsonObject report = buildFinalReport(extraSuggestions);
    emit finalReportReady(report);

    setState(State::Finished);
    emit interviewFinished(m_interviewId);
}

QJsonObject InterviewManager::buildFinalReport(const QJsonArray &extraSuggestions) const
{
    QJsonArray suggestions = extraSuggestions;
    QJsonArray perQuestion;
    for (const auto &r : m_results) {
        perQuestion.append(QJsonObject{
            {"question", r.question},
            {"answer", r.answerText},
            {"accuracy", r.accuracy},
            {"completeness", r.completeness},
            {"clarity", r.clarity},
            {"confidence", r.confidence},
            {"summary", r.summary},
        });
        if (r.accuracy > 0 && r.accuracy < 6) {
            suggestions.append(QString("Revisit: %1").arg(r.question));
        }
    }

    double eyeContactPct = m_camera ? m_camera->eyeContactPercentage() : 0.0;

    return QJsonObject{
        {"category", m_category},
        {"difficulty", m_difficulty},
        {"eyeContactPct", eyeContactPct},
        {"dominantEmotion", m_camera ? m_camera->dominantEmotion() : "N/A"},
        {"leaningPct", m_camera ? m_camera->leaningPercentage() : 0.0},
        {"headDownPct", m_camera ? m_camera->headDownPercentage() : 0.0},
        {"excessiveMovementPct", m_camera ? m_camera->excessiveMovementPercentage() : 0.0},
        {"aiSuggestions", suggestions},
        {"perQuestion", perQuestion},
    };
}

void InterviewManager::stop()
{
    m_countdownTimer.stop();
    if (m_camera) m_camera->stop();
    setState(State::Idle);
}

void InterviewManager::onTick()
{
    ++m_secondsElapsed;
    emit timeRemainingChanged(m_durationSeconds - m_secondsElapsed);
    if (m_secondsElapsed >= m_durationSeconds && m_state != State::Evaluating && m_state != State::Finished) {
        finishInterview();
    }
}

void InterviewManager::setState(State s)
{
    m_state = s;
    emit stateChanged(s);
}

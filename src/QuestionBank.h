#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QRandomGenerator>

/**
 * Provides the AI Question Generator's question pool. This is a static bank
 * (matching the categories/examples from the project spec) rather than a
 * live LLM call, so interviews can start instantly without waiting on
 * Ollama/OpenAI for every single question — llm_feedback.py is reserved for
 * *evaluating* answers, which is where the LLM adds the most value.
 *
 * Swap `questionsFor()` for a PythonBridge call to a dedicated
 * question_generator.py if you want fully dynamic questions later; the
 * calling code (InterviewManager) doesn't need to change.
 */
class QuestionBank {
public:
    static QStringList questionsFor(const QString &category, const QString &difficulty, int count)
    {
        QStringList pool = allQuestionsFor(category);
        if (pool.isEmpty()) pool = allQuestionsFor("Mixed");

        // Difficulty just changes how deep into the pool we shuffle from for
        // now — a real implementation would tag each question with a level.
        Q_UNUSED(difficulty);

        QStringList shuffled = pool;
        for (int i = shuffled.size() - 1; i > 0; --i) {
            int j = QRandomGenerator::global()->bounded(i + 1);
            shuffled.swapItemsAt(i, j);
        }

        return shuffled.mid(0, qMin(count, shuffled.size()));
    }

    // Reference answers used by llm_feedback.py's "reference_answer" field.
    // Intentionally short — the LLM is expected to grade using its own
    // domain knowledge too, this just anchors it.
    static QString referenceAnswerFor(const QString &question)
    {
        return s_referenceAnswers.value(question, QString());
    }

private:
    static QStringList allQuestionsFor(const QString &category)
    {
        static const QMap<QString, QStringList> bank = {
            {"Data Structures", {
                "Explain BFS.",
                "What is a Segment Tree?",
                "Difference between DFS and BFS?",
                "What is a hash collision and how is it resolved?",
                "Explain the difference between a stack and a queue."
            }},
            {"Algorithms", {
                "Explain BFS.",
                "Difference between DFS and BFS?",
                "What is dynamic programming and when would you use it?",
                "Explain time complexity of quicksort in best/worst case."
            }},
            {"OOP", {
                "Explain Polymorphism.",
                "What is a Virtual Function?",
                "What is Multiple Inheritance and what problems can it cause?",
                "Explain the difference between abstraction and encapsulation."
            }},
            {"DBMS", {
                "Explain the ACID properties.",
                "What is a SQL JOIN? Name the different types.",
                "What is a database index and why does it speed up queries?",
                "Explain normalization and why it matters."
            }},
            {"Operating System", {
                "What is a deadlock and how can it be prevented?",
                "Explain paging.",
                "Difference between a thread and a process?",
                "Explain the difference between preemptive and non-preemptive scheduling."
            }},
            {"Computer Networks", {
                "Difference between TCP and UDP?",
                "What is DNS and how does it work?",
                "Difference between HTTP and HTTPS?",
                "What happens when you type a URL into a browser and hit enter?"
            }},
            {"Software Engineering", {
                "Explain the software development life cycle (SDLC).",
                "What is the difference between Agile and Waterfall?",
                "What is a design pattern? Give an example.",
                "How do you approach writing unit tests for a new feature?"
            }},
            {"HR Interview", {
                "Tell me about yourself.",
                "Why should we hire you?",
                "What is your greatest strength and weakness?",
                "Describe a time you disagreed with a teammate. How did you resolve it?"
            }},
        };

        if (category == "Mixed") {
            QStringList combined;
            for (const auto &list : bank) combined += list;
            return combined;
        }
        return bank.value(category);
    }

    static inline const QMap<QString, QString> s_referenceAnswers = {
        {"Explain BFS.",
         "Breadth-first search explores a graph level by level using a queue, "
         "visiting all neighbors of a node before moving to the next depth level."},
        {"What is a Segment Tree?",
         "A binary tree used to answer range queries (e.g. sum, min) and "
         "perform point/range updates in O(log n) time."},
        {"Explain Polymorphism.",
         "The ability of an object to take on many forms — typically via "
         "virtual functions/method overriding, allowing a base-class pointer "
         "to call the correct derived-class implementation at runtime."},
        {"Explain the ACID properties.",
         "Atomicity, Consistency, Isolation, Durability — the guarantees a "
         "database transaction provides."},
        {"What is a deadlock and how can it be prevented?",
         "A state where two or more processes are waiting on each other's "
         "resources indefinitely; prevented via resource ordering, timeouts, "
         "or avoiding circular wait."},
        {"Difference between TCP and UDP?",
         "TCP is connection-oriented, reliable, and ordered; UDP is "
         "connectionless, faster, but does not guarantee delivery or order."},
    };
};

"""
recommendation.py — analyzes a user's interview history (per-category
scores) to find weak topics and suggest practice problems / study topics /
difficulty level.

Server mode:
    request:  {
        "cmd": "recommend",
        "category_scores": {"DSA": 62, "OOP": 88, "DBMS": 55, "OS": 74}
    }
    response: {
        "weak_topics": ["DBMS", "DSA"],
        "suggestions": [
            {"topic": "DBMS", "focus": "SQL JOIN, Indexing", "difficulty": "Medium"},
            {"topic": "DSA", "focus": "Graph, Dynamic Programming", "difficulty": "Easy"}
        ]
    }

This is a rule-based recommender (no external model needed) so it works
fully offline; swap in an LLM call (reusing llm_feedback.py's backend) for
richer, personalized suggestions later.
"""

import argparse
import json

from protocol import read_requests, send_response, log

_WEAK_THRESHOLD = 70

_TOPIC_FOCUS_AREAS = {
    "DSA": ["Graph", "Dynamic Programming", "Segment Tree", "Binary Search"],
    "Algorithms": ["Greedy", "Divide and Conquer", "Backtracking"],
    "OOP": ["Polymorphism", "Virtual Functions", "Multiple Inheritance"],
    "DBMS": ["SQL JOIN", "Indexing", "Normalization", "ACID Properties"],
    "Operating System": ["Deadlock", "Paging", "Thread vs Process", "Scheduling"],
    "Computer Networks": ["TCP vs UDP", "DNS", "HTTP vs HTTPS", "Subnetting"],
    "Software Engineering": ["SDLC Models", "Design Patterns", "Testing"],
    "HR Interview": ["Behavioral Questions", "STAR Method"],
}


def _difficulty_for_score(score: float) -> str:
    if score < 50:
        return "Easy"
    if score < 75:
        return "Medium"
    return "Hard"


def recommend(category_scores: dict) -> dict:
    weak_topics = sorted(
        (cat for cat, score in category_scores.items() if score < _WEAK_THRESHOLD),
        key=lambda cat: category_scores[cat],
    )

    suggestions = []
    for topic in weak_topics:
        score = category_scores[topic]
        focus_areas = _TOPIC_FOCUS_AREAS.get(topic, [])
        suggestions.append({
            "topic": topic,
            "focus": ", ".join(focus_areas[:3]) if focus_areas else "General review",
            "difficulty": _difficulty_for_score(score),
            "current_score": score,
        })

    return {"weak_topics": weak_topics, "suggestions": suggestions}


def run_server():
    log("recommendation.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "recommend":
            result = recommend(request.get("category_scores", {}))
        else:
            result = {"error": f"unknown cmd: {cmd}"}
        send_response(result)


def run_selftest():
    sample = {"DSA": 62, "OOP": 88, "DBMS": 55, "Operating System": 74}
    print(json.dumps(recommend(sample), indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--serve", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    if args.serve:
        run_server()
    else:
        run_selftest()

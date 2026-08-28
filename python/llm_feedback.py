"""
llm_feedback.py — evaluates a candidate's spoken answer against a reference
answer using a local LLM via Ollama (default) or the OpenAI API, returning
structured scores: accuracy, completeness, clarity, confidence.

Server mode:
    request:  {
        "cmd": "evaluate",
        "question": "Explain BFS.",
        "answer": "...",
        "reference_answer": "..."   // optional
    }
    response: {
        "accuracy": 8, "completeness": 7, "clarity": 9, "confidence": 7,
        "summary": "..."
    }

Backend is selected by the AI_BACKEND env var ("ollama" | "openai"),
defaulting to "ollama" so the project works fully offline.
"""

import argparse
import json
import os
import re

import requests

from protocol import read_requests, send_response, log

_OLLAMA_URL = "http://localhost:11434/api/generate"
_OLLAMA_MODEL = os.environ.get("OLLAMA_MODEL", "llama3.2")
_OPENAI_URL = "https://api.openai.com/v1/chat/completions"
_OPENAI_MODEL = os.environ.get("OPENAI_MODEL", "gpt-4o-mini")

_PROMPT_TEMPLATE = """You are an expert technical interviewer grading a candidate's answer.

Question: {question}
Reference answer (may be partial, use your own expertise too): {reference}
Candidate's answer (transcribed from speech, may have minor errors): {answer}

Score the candidate's answer from 1-10 on each of: accuracy, completeness, clarity, confidence.
Then give a 1-2 sentence summary of feedback.

Respond ONLY with valid JSON in this exact shape, no other text:
{{"accuracy": <int>, "completeness": <int>, "clarity": <int>, "confidence": <int>, "summary": "<string>"}}
"""


def _extract_json(text: str) -> dict:
    match = re.search(r"\{.*\}", text, re.DOTALL)
    if not match:
        raise ValueError(f"no JSON object found in LLM response: {text[:200]}")
    return json.loads(match.group(0))


def _call_ollama(prompt: str) -> str:
    resp = requests.post(
        _OLLAMA_URL,
        json={
            "model": _OLLAMA_MODEL,
            "prompt": prompt,
            "stream": False,
            # The response is a short fixed-shape JSON object — capping
            # num_predict stops the model from rambling past that and
            # meaningfully speeds up CPU-only inference.
            "options": {"num_predict": 200, "temperature": 0.2},
        },
        timeout=120,
    )
    resp.raise_for_status()
    return resp.json().get("response", "")


def _call_openai(prompt: str) -> str:
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        raise RuntimeError("OPENAI_API_KEY not set")
    resp = requests.post(
        _OPENAI_URL,
        headers={"Authorization": f"Bearer {api_key}"},
        json={
            "model": _OPENAI_MODEL,
            "messages": [{"role": "user", "content": prompt}],
            "temperature": 0.2,
        },
        timeout=60,
    )
    resp.raise_for_status()
    return resp.json()["choices"][0]["message"]["content"]


def evaluate_answer(question: str, answer: str, reference_answer: str = "") -> dict:
    prompt = _PROMPT_TEMPLATE.format(question=question, reference=reference_answer or "N/A", answer=answer)

    backend = os.environ.get("AI_BACKEND", "ollama")
    raw = _call_openai(prompt) if backend == "openai" else _call_ollama(prompt)

    try:
        return _extract_json(raw)
    except (ValueError, json.JSONDecodeError) as exc:
        return {"error": f"could not parse LLM output: {exc}", "raw": raw}


def run_server():
    log("llm_feedback.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "evaluate":
            try:
                result = evaluate_answer(
                    request.get("question", ""),
                    request.get("answer", ""),
                    request.get("reference_answer", ""),
                )
            except Exception as exc:  # noqa: BLE001
                result = {"error": str(exc)}
        else:
            result = {"error": f"unknown cmd: {cmd}"}
        send_response(result)


def run_selftest():
    print("llm_feedback.py loaded OK.")
    print(f"Backend: {os.environ.get('AI_BACKEND', 'ollama')}")
    print("Make sure `ollama run llama3.2` (or OPENAI_API_KEY) is available, then try:")
    print('  python llm_feedback.py --question "Explain BFS." --answer "It visits nodes level by level using a queue."')


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--serve", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--question")
    parser.add_argument("--answer")
    parser.add_argument("--reference", default="")
    args = parser.parse_args()

    if args.serve:
        run_server()
    elif args.question and args.answer:
        print(json.dumps(evaluate_answer(args.question, args.answer, args.reference), indent=2))
    else:
        run_selftest()

"""
Shared JSON stdin/stdout protocol helper for InterviewIQ's Python AI
services. Each AI module (face_detection.py, eye_tracking.py, ...) can
either be run standalone for testing (--selftest) or as a long-lived
"server" process (--serve) that InterviewIQ's PythonBridge (C++/QProcess)
talks to via line-delimited JSON.

See docs/PROTOCOL.md for the full message contract.
"""

import sys
import json


def read_requests():
    """Yields one parsed JSON dict per line from stdin, forever."""
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            yield json.loads(line)
        except json.JSONDecodeError as exc:
            send_response({"error": f"invalid JSON: {exc}"})


def send_response(payload: dict) -> None:
    """Writes one JSON object as a line to stdout and flushes immediately
    so the C++ side sees it right away (QProcess reads line-buffered)."""
    sys.stdout.write(json.dumps(payload) + "\n")
    sys.stdout.flush()


def log(message: str) -> None:
    """Diagnostic logging goes to stderr, never stdout (stdout is reserved
    for the JSON protocol)."""
    print(message, file=sys.stderr, flush=True)

# C++ ↔ Python JSON Protocol

Each Python AI module in `python/` can run in two modes:

- `python <module>.py --selftest` — runs standalone against your webcam/mic
  for local testing, prints results to stdout as human-readable text.
- `python <module>.py --serve` — long-lived process, reads one JSON request
  per line from **stdin**, writes one JSON response per line to **stdout**.
  This is the mode `PythonBridge` (C++) launches via `QProcess`.

**Rule:** stdout is reserved exclusively for protocol JSON. All diagnostic
logging goes to stderr (see `protocol.log()`), so the C++ side's line parser
never chokes on stray print statements.

## Message shapes

### face_detection.py
```
→ {"cmd": "detect_frame", "frame_path": "/tmp/frame_0001.jpg"}
← {"face_present": true, "face_count": 1, "multiple_faces": false, "boxes": [[x,y,w,h]]}
```

### eye_tracking.py
```
→ {"cmd": "detect_frame", "frame_path": "/tmp/frame_0001.jpg"}
← {"looking_at_camera": true, "gaze_offset_x": 0.03}
```

### emotion_detection.py
```
→ {"cmd": "detect_frame", "frame_path": "/tmp/frame_0001.jpg"}
← {"emotion": "Confident", "confidence": 0.62}
```

### speech_to_text.py
```
→ {"cmd": "transcribe", "audio_path": "/tmp/answer_003.wav", "model_size": "tiny"}
← {
    "text": "BFS visits nodes level by level using a queue...",
    "duration_seconds": 42.1,
    "words_per_minute": 132.4,
    "pause_count": 3,
    "filler_count": 2,
    "fillers_found": ["umm", "like"]
  }
```

### llm_feedback.py
```
→ {
    "cmd": "evaluate",
    "question": "Explain BFS.",
    "answer": "BFS visits nodes level by level using a queue...",
    "reference_answer": "Breadth-first search explores neighbors before going deeper..."
  }
← {"accuracy": 8, "completeness": 7, "clarity": 9, "confidence": 7, "summary": "Solid core explanation, missing time complexity."}
```
Backend controlled by the `AI_BACKEND` env var (`ollama` default, or `openai`
with `OPENAI_API_KEY` set).

### recommendation.py
```
→ {"cmd": "recommend", "category_scores": {"DSA": 62, "OOP": 88, "DBMS": 55}}
← {
    "weak_topics": ["DBMS", "DSA"],
    "suggestions": [
      {"topic": "DBMS", "focus": "SQL JOIN, Indexing, Normalization", "difficulty": "Medium", "current_score": 55},
      {"topic": "DSA", "focus": "Graph, Dynamic Programming, Segment Tree", "difficulty": "Easy", "current_score": 62}
    ]
  }
```

## Error convention
Any module may respond with `{"error": "<message>"}` instead of its normal
payload. `PythonBridge::resultReceived` still fires for these — check for an
`"error"` key before treating a response as a success.

## Why frame *paths* instead of raw bytes over stdin?
Keeps the JSON protocol simple (no base64 blobs) and lets CameraController
reuse the same temp-file mechanism for all three CV modules per frame. If
you outgrow this (e.g. need higher frame rates), switch to a shared-memory
ring buffer or a local Unix domain / named-pipe socket carrying raw frames,
and keep this JSON channel just for control messages.

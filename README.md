# 🚀InterviewIQ – AI Powered Technical Interview Simulator

A hybrid **C++/Qt6** desktop application + **Python** AI microservice suite that
simulates a real software-engineering technical interview: it asks questions,
watches you through the camera, listens to your voice, evaluates your answers
with an LLM, and generates a full analytics + PDF report.

This repository is a **starter scaffold**. The architecture, build system,
database schema, class skeletons, and Python module stubs are all wired up
and ready to be filled in module by module (see `docs/SPEC.md` for the full
feature list this scaffold was generated from).

---
---

## ✨ Features

- 🎯 AI-based technical interview simulation
- 📷 Real-time face detection and eye-contact analysis
- 😊 Emotion and confidence analysis
- 🎤 Speech-to-text processing using Whisper
- 🤖 LLM-based answer evaluation and feedback
- 📊 Performance analytics with charts
- 📄 PDF interview report generation
- 🗂️ Interview history tracking
- 🔄 C++ ↔ Python communication using JSON

---

## Architecture

```
┌─────────────────────────────┐        JSON over stdio/socket        ┌──────────────────────────┐
│        C++ / Qt6 App        │  <--------------------------------->  │      Python AI Services   │
│  UI, DB, Charts, PDF, Flow  │                                        │  CV, Speech, LLM, NLP     │
└─────────────────────────────┘                                        └──────────────────────────┘
          │
          ▼
      SQLite DB
```

- The **C++/Qt app** owns the UI, the interview workflow/state machine, the
  SQLite database, chart rendering (Qt Charts) and PDF report generation
  (QPdfWriter).
- The **Python side** owns anything AI/CV/NLP: face detection, eye tracking,
  emotion recognition, speech-to-text (Whisper), and LLM-based answer
  evaluation (Ollama/OpenAI).
- Communication happens via `QProcess`, exchanging line-delimited JSON on
  stdin/stdout (see `docs/PROTOCOL.md`). This keeps the two runtimes fully
  decoupled — you can swap the Python backend for a local HTTP server later
  without touching the C++ side.

## Repository layout

```
InterviewIQ/
├── CMakeLists.txt              # Qt6 build configuration
├── requirements.txt            # top-level pointer to python/requirements.txt
├── src/                        # C++ / Qt application
│   ├── main.cpp
│   ├── LoginManager.h/.cpp
│   ├── DashboardWidget.h/.cpp
│   ├── InterviewManager.h/.cpp
│   ├── DatabaseManager.h/.cpp
│   ├── ChartManager.h/.cpp
│   ├── PdfReportGenerator.h/.cpp
│   ├── SettingsManager.h/.cpp
│   ├── HistoryManager.h/.cpp
│   ├── CameraController.h/.cpp
│   └── PythonBridge.h/.cpp     # QProcess <-> JSON bridge to python/
├── python/                     # AI modules (independent, testable scripts)
│   ├── requirements.txt
│   ├── face_detection.py
│   ├── eye_tracking.py
│   ├── emotion_detection.py
│   ├── speech_to_text.py
│   ├── llm_feedback.py
│   └── recommendation.py
├── database/
│   └── schema.sql              # SQLite schema (Users, Interview, Results, History)
├── docs/
│   ├── SPEC.md                 # full original feature spec
│   ├── SETUP.md                # step-by-step environment setup (Windows)
│   └── PROTOCOL.md             # JSON message contract between C++ and Python
└── resources/                  # icons, .qrc, stylesheets (empty, add your own)
```

## Quick start

### 1. C++ / Qt side
```bash
cmake -S . -B build
cmake --build build
```
Requires Qt6 (Widgets, Charts, Multimedia, Sql), a C++17/20 compiler, and CMake.
See `docs/SETUP.md` for full step-by-step Windows install instructions.

### 2. Python AI side
```bash
cd python
pip install -r requirements.txt --break-system-packages   # or use a venv
python face_detection.py --selftest
```

### 3. Database
```bash
sqlite3 interviewiq.db < database/schema.sql
```

## Development order (recommended)

1. **Phase 1** — Visual Studio / Qt / Git, get `CMakeLists.txt` building an empty window.
2. **Phase 2** — Python + OpenCV + MediaPipe, get `face_detection.py` and
   `eye_tracking.py` printing JSON to stdout standalone.
3. **Phase 3** — Whisper + Ollama, get `speech_to_text.py` and `llm_feedback.py` working standalone.
4. **Phase 4** — Wire `PythonBridge` (QProcess) to call the Python scripts and
   parse their JSON, then persist results via `DatabaseManager`.
5. **Phase 5** — Charts, PDF report, history, achievements, leaderboard.

## Status

This is now a working end-to-end flow, not just a shell:

- **Login/Register** — real forms, SHA-256 password hashing, SQLite-backed.
- **Dashboard** — real stat cards (total interviews, average score, strong/weak
  subject, streak, practice hours, last result, suggested goal) computed from
  live SQL queries, plus a progress line chart.
- **Interview flow** — pick category/difficulty/duration → questions pulled
  from a built-in question bank → camera starts and samples frames every 2s,
  dispatching them to `face_detection.py` / `eye_tracking.py` /
  `emotion_detection.py` over `PythonBridge` → typed answers are sent to
  `llm_feedback.py` for scoring → a final report screen shows eye-contact %,
  dominant emotion, and weak-topic suggestions, with a **PDF export** button
  that embeds a real Qt Charts score-breakdown pie.
- **History** — a real filterable table backed by SQL joins across
  Interview/Results/History.

**Still open / good next steps:**
- Voice answers are currently typed, not recorded — `InterviewScreen.h` has
  a comment describing exactly how to swap in `QAudioInput`/`QMediaRecorder`
  → `speech_to_text.py` without touching `InterviewManager`.
- Posture detection, coding-round module, achievements/leaderboard UI, and
  settings screen are still skeletons — see the `TODO`s in `SettingsManager`
  and the spec's modules 8, 12, 19, 20 for what's left.
- Password reset (`LoginManager::requestPasswordReset`) is a stub — an
  offline desktop app needs a different approach than emailing a reset link
  (e.g. a security question stored at registration).


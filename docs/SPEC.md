# InterviewIQ — Full Feature Specification

This is the source spec this scaffold was generated from.

## Overview
InterviewIQ is an AI-based desktop application that recreates a real
software-engineering technical interview. It analyzes a student's technical
knowledge, communication skill, confidence, eye contact, emotion, speaking
ability, and coding performance, and produces a detailed feedback report.

**Target users:** CSE students, job seekers, internship candidates, competitive programmers.

## Technology Stack
- **Desktop app:** C++17/20, Qt6 (Widgets, Charts, Multimedia), SQLite, CMake
- **AI modules:** Python, OpenCV, MediaPipe, Whisper, Ollama/OpenAI API, NumPy
- **Communication:** JSON over QProcess / local API

## Modules

1. **User Authentication** — registration, secure login, forgot password, profile management, password change.
2. **Dashboard** — total interviews, average score, strong/weak subjects, daily streak, practice hours, last result, upcoming goal.
3. **Interview Categories** — DSA, Algorithms, OOP, DBMS, OS, CN, SE, HR, Mixed; difficulty Easy/Medium/Hard.
4. **AI Question Generator** — dynamic questions per category (e.g. BFS, Segment Tree, Polymorphism, ACID, Deadlock, TCP vs UDP, HR behavioral questions).
5. **Camera Monitoring** (OpenCV) — face present / multiple faces / face missing, live preview.
6. **Eye Contact Detection** (MediaPipe) — eye direction, looking away vs. at camera, eye contact %, timeline.
7. **Emotion Recognition** — Happy, Neutral, Confident, Nervous, Sad, Confused; emotion graph over time.
8. **Posture Detection** — sitting properly, leaning, head down, excessive movement.
9. **Voice Recognition** — microphone → Whisper → real-time speech-to-text.
10. **Speaking Analysis** — words per minute, speaking speed, pause count, filler-word detection (umm, ah, like, basically).
11. **AI Answer Evaluation** — LLM compares question / student answer / reference answer → accuracy, completeness, clarity, confidence.
12. **Coding Interview Module** — built-in editor (C++, Python optional), compile/run, sample + hidden test cases, execution time, memory usage.
13. **Timer** — 15/30/45/60 minute interviews with countdown.
14. **Interview Analytics** — technical/communication/confidence/AI/overall scores; pie, bar, line charts.
15. **Weak Topic Recommendation** — AI finds weak topics (e.g. Graph, DP, SQL JOIN) and suggests practice problems, study topics, difficulty level.
16. **History** — date, time, category, duration, score, feedback, stored in SQLite.
17. **PDF Report** — candidate info, scores, technical/communication analysis, eye contact, emotion, speaking speed, AI suggestions, final rating.
18. **Progress Tracker** — weekly/monthly/overall score improvement, practice hours, interview count.
19. **Achievement System** — badges: First Interview, 10 Interviews, Excellent Communication, DSA Expert, Interview Master.
20. **Leaderboard (optional)** — rank by total score, practice time, interview count.
21. **Settings** — camera, microphone, theme, language, AI model, difficulty, notifications.

## Database Tables
- **Users**: ID, Name, Email, Password(Hash), Created Date
- **Interview**: Interview ID, User ID, Category, Difficulty, Duration, Date
- **Results**: Technical Score, Communication Score, Confidence, Eye Contact, Speaking Speed, Overall Score
- **History**: Interview ID, Result, Feedback

## Python AI Modules
- `face_detection.py` — face / multiple-faces detection
- `eye_tracking.py` — eye contact / looking-away detection
- `emotion_detection.py` — Happy / Neutral / Nervous / etc.
- `speech_to_text.py` — Whisper transcription
- `llm_feedback.py` — LLM answer evaluation
- `recommendation.py` — weak-topic + study suggestions

## C++ Modules
Login Manager, Dashboard, Interview Manager, Database Manager, Chart Manager,
PDF Generator, Settings Manager, History Manager, JSON Communication, Camera Controller.

## System Workflow
```
Start Application → Login/Register → Dashboard → Select Interview Type
→ Initialize Camera & Microphone → AI Generates Questions → User Gives Answer
   ├── Voice → Speech-to-Text
   ├── Camera → Face Detection
   ├── Camera → Eye Tracking
   └── Camera → Emotion Detection
→ AI Evaluates Performance → Generate Charts → Generate PDF Report
→ Save to Database → Dashboard Updated
```

## Future / Advanced Features
Online multiplayer mock interviews, 3D HR avatar interviewer, voice emotion
detection, resume analyzer, LinkedIn resume import, AI follow-up questions,
cloud backup, interview replay, multi-language support, dark/light theme,
interview recording, AI personalized learning plan.

## Why this project is worth building
It's most valuable when the AI is *one feature among many*, not the whole
project — i.e. when you're also doing the real engineering: the interview
workflow, database design, Qt UI, analytics dashboard, eye-contact tracking,
PDF report generation, and the C++ ↔ Python integration layer. A project
that's just "user → ChatGPT API → answer → end" won't showcase much beyond
API-calling. This scaffold is built so the AI calls are cleanly isolated
behind `PythonBridge`/JSON, leaving the substantial engineering — UI, state
machine, database, charts, reports — as the part you build and can speak to
in an interview.

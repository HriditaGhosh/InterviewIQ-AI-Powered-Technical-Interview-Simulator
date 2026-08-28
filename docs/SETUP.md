# Environment Setup (Windows)

Don't install everything on day one — install alongside the project, phase
by phase (see suggested order at the bottom).

## 1. Visual Studio Community (C++ compiler)
Download: https://visualstudio.microsoft.com/downloads/

During install, select **Desktop development with C++**. This brings in the
MSVC compiler, CMake, and the Windows SDK.

## 2. Qt 6
Download: https://www.qt.io/download (or the offline installers page)

Select:
- Qt 6.x
- MSVC kit (matching your Visual Studio version)
- Qt Creator
- Qt Charts
- Qt Multimedia
- Qt SVG (needed for the app icon in resources/)

## 3. Git
Download: https://git-scm.com/downloads

## 4. Python
Download: https://www.python.org/downloads/

⚠️ Tick **"Add Python to PATH"** during install. Verify with:
```
python --version
```

## 5. Python AI packages
```
pip install opencv-python
pip install mediapipe
pip install numpy
pip install openai-whisper
pip install torch torchvision torchaudio   # CPU version is fine to start
pip install requests
```
Or simply:
```
cd python
pip install -r requirements.txt
```

## 6. Ollama (local LLM)
Download: https://ollama.com/

Then in a terminal:
```
ollama run llama3.2
```
or
```
ollama run mistral
```

## 6b. FFmpeg (required by Whisper)
`openai-whisper` shells out to `ffmpeg` to decode audio, including the
`.wav` files `AudioRecorder` records — without it, transcription fails.
Download: https://www.gyan.dev/ffmpeg/builds/ (Windows builds) or
https://ffmpeg.org/download.html, then add the `bin` folder to PATH.
Verify with:
```
ffmpeg -version
```

## 6c. MinGW-w64 (for the Coding Round module's compile/run judge)
The Coding Round screen compiles submitted C++ with `g++` found on PATH —
separate from the MSVC compiler used to build InterviewIQ itself, since
MSVC's `cl.exe` needs a `vcvars`-initialized shell to run standalone.
Easiest install: https://www.msys2.org/, then in the MSYS2 shell:
```
pacman -S mingw-w64-ucrt-x86_64-gcc
```
Add `<msys64>\ucrt64\bin` to PATH, then verify with:
```
g++ --version
```

## 7. SQLite
Download: https://www.sqlite.org/download.html

(Qt's SQLite driver plugin often ships with Qt already, so a separate
install may not be needed — check `qt-cmake --list-plugins` or the Qt
Maintenance Tool.)

## 8. CMake
Usually already installed by the Visual Studio C++ workload. Verify:
```
cmake --version
```

## 9. VS Code (optional)
Download: https://code.visualstudio.com/

Recommended extensions: C++, Python, CMake, SQLite, GitHub Copilot (optional).

## JSON & QProcess
No installation needed — both are built into Qt (`QJsonObject`,
`QJsonDocument`, `QProcess`).

## Final checklist

| Software | Required? |
|---|---|
| Visual Studio | ✅ |
| Qt Creator (with Charts, Multimedia, SVG) | ✅ |
| Python | ✅ |
| Git | ✅ |
| Ollama | ✅ |
| FFmpeg | ✅ (Whisper needs it) |
| MinGW-w64 (g++) | ✅ (for the Coding Round module) |
| SQLite | ✅ |
| VS Code | Optional |

## Suggested install order

- **Week 1:** Visual Studio + Qt + Git
- **Week 2:** Python + OpenCV + MediaPipe
- **Week 3:** SQLite + JSON + QProcess integration
- **Week 4:** Whisper + Ollama

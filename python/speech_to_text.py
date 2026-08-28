"""
speech_to_text.py — transcribes recorded answer audio to text using OpenAI's
Whisper, and computes basic speaking-analysis metrics (WPM, pause count,
filler-word count) from the transcription's word timestamps.

Server mode:
    request:  {"cmd": "transcribe", "audio_path": "/tmp/answer.wav"}
    response: {
        "text": "...",
        "words_per_minute": 132.4,
        "pause_count": 3,
        "filler_count": 2,
        "fillers_found": ["umm", "like"]
    }

Note: loading the Whisper model is slow (several seconds) — this is done
once at process start in server mode, not per-request.
"""

import argparse
import re

from protocol import read_requests, send_response, log

_FILLER_WORDS = {"umm", "um", "uh", "ah", "like", "basically", "actually", "you know"}

_model = None


def _get_model(size: str = "tiny"):
    global _model
    if _model is None:
        import whisper  # imported lazily so --selftest without audio still works fast
        log(f"speech_to_text.py: loading Whisper model '{size}' (this can take a while)...")
        _model = whisper.load_model(size)
    return _model


def transcribe(audio_path: str, model_size: str = "tiny") -> dict:
    model = _get_model(model_size)
    result = model.transcribe(
        audio_path,
        word_timestamps=True,
        fp16=False,
        # Whisper's default temperature is a fallback ladder (0, 0.2, 0.4,
        # ...) that re-decodes up to 6x if quality thresholds aren't met —
        # a single fixed 0.0 skips that retry loop entirely. For short,
        # single-utterance practice answers (not noisy/ambiguous audio),
        # this is a large, safe speed win with only a small accuracy
        # trade-off on genuinely hard audio.
        temperature=0.0,
        # Skips Whisper's automatic language-detection pass (which decodes
        # an extra window just to guess the language) — set to None instead
        # of "en" if answers are commonly in another language.
        language="en",
    )

    text = result.get("text", "").strip()
    segments = result.get("segments", [])

    words = []
    for seg in segments:
        for w in seg.get("words", []):
            words.append(w)

    duration_sec = segments[-1]["end"] if segments else 0.0
    word_count = len(text.split())
    wpm = (word_count / duration_sec * 60.0) if duration_sec > 0 else 0.0

    # Pause detection: gaps between consecutive words longer than 0.6s.
    pause_count = 0
    for i in range(1, len(words)):
        gap = words[i]["start"] - words[i - 1]["end"]
        if gap > 0.6:
            pause_count += 1

    fillers_found = [
        w for w in re.findall(r"[a-zA-Z']+", text.lower()) if w in _FILLER_WORDS
    ]

    return {
        "text": text,
        "duration_seconds": round(duration_sec, 1),
        "words_per_minute": round(wpm, 1),
        "pause_count": pause_count,
        "filler_count": len(fillers_found),
        "fillers_found": fillers_found,
    }


def run_server():
    log("speech_to_text.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "transcribe":
            try:
                result = transcribe(request.get("audio_path", ""), request.get("model_size", "tiny"))
            except Exception as exc:  # noqa: BLE001 — surface any failure to the C++ side
                result = {"error": str(exc)}
        else:
            result = {"error": f"unknown cmd: {cmd}"}
        send_response(result)


def run_selftest():
    print("speech_to_text.py loaded OK. Run with an audio file to test transcription:")
    print("  python speech_to_text.py --transcribe path/to/answer.wav")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--serve", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--transcribe", metavar="AUDIO_PATH")
    args = parser.parse_args()

    if args.serve:
        run_server()
    elif args.transcribe:
        import json
        print(json.dumps(transcribe(args.transcribe), indent=2))
    else:
        run_selftest()

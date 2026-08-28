"""
emotion_detection.py — classifies the candidate's facial expression into
one of: Happy, Neutral, Confident, Nervous, Sad, Confused.

This scaffold ships a lightweight heuristic based on MediaPipe Face Mesh
landmark geometry (mouth/eyebrow shape) so the pipeline runs end-to-end
with zero extra model downloads. Swap `classify_emotion()` for a proper
trained classifier (e.g. an ONNX/FER model) when you're ready — the
server protocol below won't need to change.

Server mode:
    request:  {"cmd": "detect_frame", "frame_path": "/tmp/frame.jpg"}
    response: {"emotion": "Confident", "confidence": 0.62}
"""

import argparse

import cv2
import numpy as np

from protocol import read_requests, send_response, log
from mediapipe_compat import create_face_landmarker, to_mp_image

_face_landmarker = create_face_landmarker(num_faces=1)

# A few landmark indices used for the heuristic.
_MOUTH_LEFT, _MOUTH_RIGHT = 61, 291
_MOUTH_TOP, _MOUTH_BOTTOM = 13, 14
_LEFT_BROW, _RIGHT_BROW = 105, 334
_LEFT_EYE_TOP, _RIGHT_EYE_TOP = 159, 386

_EMOTIONS = ["Happy", "Neutral", "Confident", "Nervous", "Sad", "Confused"]


def _pt(landmarks, idx, w, h):
    lm = landmarks[idx]
    return np.array([lm.x * w, lm.y * h])


def classify_emotion(frame_bgr) -> dict:
    h, w = frame_bgr.shape[:2]
    results = _face_landmarker.detect(to_mp_image(frame_bgr))

    if not results.face_landmarks:
        return {"emotion": "Neutral", "confidence": 0.0, "reason": "no_face"}

    lm = results.face_landmarks[0]

    mouth_width = np.linalg.norm(_pt(lm, _MOUTH_LEFT, w, h) - _pt(lm, _MOUTH_RIGHT, w, h))
    mouth_height = np.linalg.norm(_pt(lm, _MOUTH_TOP, w, h) - _pt(lm, _MOUTH_BOTTOM, w, h))
    brow_gap = np.linalg.norm(_pt(lm, _LEFT_BROW, w, h) - _pt(lm, _RIGHT_BROW, w, h))
    eye_brow_dist = (
        np.linalg.norm(_pt(lm, _LEFT_BROW, w, h) - _pt(lm, _LEFT_EYE_TOP, w, h))
        + np.linalg.norm(_pt(lm, _RIGHT_BROW, w, h) - _pt(lm, _RIGHT_EYE_TOP, w, h))
    ) / 2

    # Very rough, explainable heuristics — replace with a trained model.
    mouth_open_ratio = mouth_height / (mouth_width or 1.0)

    if mouth_open_ratio > 0.35:
        emotion = "Happy"
        confidence = min(1.0, mouth_open_ratio)
    elif eye_brow_dist < (0.02 * h):
        emotion = "Nervous"
        confidence = 0.5
    elif brow_gap < (0.18 * w):
        emotion = "Confused"
        confidence = 0.4
    else:
        emotion = "Neutral"
        confidence = 0.5

    return {"emotion": emotion, "confidence": round(float(confidence), 2)}


def run_server():
    log("emotion_detection.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "detect_frame":
            frame = cv2.imread(request.get("frame_path", ""))
            result = {"error": "could not read frame"} if frame is None else classify_emotion(frame)
        else:
            result = {"error": f"unknown cmd: {cmd}"}
        send_response(result)


def run_selftest():
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("No webcam found — skipping live capture, but MediaPipe FaceLandmarker loaded OK.")
        return
    for i in range(5):
        ok, frame = cap.read()
        if not ok:
            break
        print(f"frame {i}: {classify_emotion(frame)}")
    cap.release()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--serve", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    if args.serve:
        run_server()
    else:
        run_selftest()

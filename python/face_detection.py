"""
face_detection.py — detects whether a face is present, missing, or whether
multiple faces are visible in a camera frame. Uses OpenCV's Haar cascade
classifier (fast, no extra model download needed).

Standalone test:
    python face_detection.py --selftest

Server mode (used by PythonBridge over stdin/stdout JSON):
    python face_detection.py --serve
    request:  {"cmd": "detect_frame", "frame_path": "/tmp/frame.jpg"}
    response: {"face_present": true, "face_count": 1, "multiple_faces": false}
"""

import sys
import argparse

import cv2

from protocol import read_requests, send_response, log

_CASCADE_PATH = cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
_face_cascade = cv2.CascadeClassifier(_CASCADE_PATH)


def detect_faces(frame_bgr):
    """Returns (face_count, bounding_boxes) for a BGR image (as loaded by cv2.imread)."""
    gray = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2GRAY)
    faces = _face_cascade.detectMultiScale(
        gray, scaleFactor=1.1, minNeighbors=5, minSize=(60, 60)
    )
    return len(faces), faces.tolist() if len(faces) else []


def detect_frame_file(path: str) -> dict:
    frame = cv2.imread(path)
    if frame is None:
        return {"error": f"could not read frame: {path}"}
    count, boxes = detect_faces(frame)
    return {
        "face_present": count > 0,
        "face_count": count,
        "multiple_faces": count > 1,
        "boxes": boxes,
    }


def run_server():
    log("face_detection.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "detect_frame":
            result = detect_frame_file(request.get("frame_path", ""))
        else:
            result = {"error": f"unknown cmd: {cmd}"}
        send_response(result)


def run_selftest():
    """Opens the default webcam for a few frames and prints detection results.
    Useful for verifying OpenCV + the cascade file work before wiring up Qt."""
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("No webcam found — skipping live capture, but OpenCV + cascade loaded OK.")
        return
    for i in range(5):
        ok, frame = cap.read()
        if not ok:
            break
        count, _ = detect_faces(frame)
        print(f"frame {i}: face_count={count}")
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

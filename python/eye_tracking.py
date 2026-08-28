"""
eye_tracking.py — estimates whether the candidate is looking at the camera
vs. looking away, using MediaPipe Face Mesh iris landmarks.

Server mode:
    request:  {"cmd": "detect_frame", "frame_path": "/tmp/frame.jpg"}
    response: {"looking_at_camera": true, "gaze_offset_x": 0.03, "gaze_offset_y": -0.01}
"""

import argparse

import cv2

from protocol import read_requests, send_response, log
from mediapipe_compat import create_face_landmarker, to_mp_image

# Iris landmark indices (unchanged — FaceLandmarker still returns the same
# 478-point topology as the old FaceMesh(refine_landmarks=True))
_LEFT_IRIS = [468, 469, 470, 471]
_RIGHT_IRIS = [473, 474, 475, 476]
_LEFT_EYE_CORNERS = (33, 133)
_RIGHT_EYE_CORNERS = (362, 263)

# How far the iris center can drift from the eye midpoint (as a fraction of
# eye width) before we call it "looking away". Tune during real testing.
_GAZE_THRESHOLD = 0.15

_face_landmarker = create_face_landmarker(num_faces=1)


def _landmark_xy(landmarks, idx, w, h):
    lm = landmarks[idx]
    return lm.x * w, lm.y * h


def estimate_gaze(frame_bgr):
    h, w = frame_bgr.shape[:2]
    results = _face_landmarker.detect(to_mp_image(frame_bgr))

    if not results.face_landmarks:
        return {"looking_at_camera": False, "reason": "no_face"}

    landmarks = results.face_landmarks[0]

    def eye_offset(iris_ids, corner_ids):
        iris_x = sum(_landmark_xy(landmarks, i, w, h)[0] for i in iris_ids) / len(iris_ids)
        c1x, _ = _landmark_xy(landmarks, corner_ids[0], w, h)
        c2x, _ = _landmark_xy(landmarks, corner_ids[1], w, h)
        eye_width = abs(c2x - c1x) or 1.0
        mid = (c1x + c2x) / 2
        return (iris_x - mid) / eye_width

    left_offset = eye_offset(_LEFT_IRIS, _LEFT_EYE_CORNERS)
    right_offset = eye_offset(_RIGHT_IRIS, _RIGHT_EYE_CORNERS)
    avg_offset = (left_offset + right_offset) / 2

    looking_at_camera = abs(avg_offset) < _GAZE_THRESHOLD

    return {
        "looking_at_camera": looking_at_camera,
        "gaze_offset_x": round(avg_offset, 4),
    }


def run_server():
    log("eye_tracking.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "detect_frame":
            frame = cv2.imread(request.get("frame_path", ""))
            result = {"error": "could not read frame"} if frame is None else estimate_gaze(frame)
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
        print(f"frame {i}: {estimate_gaze(frame)}")
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

"""
posture_detection.py — estimates sitting posture from a camera frame using
MediaPipe Pose: shoulder/head leaning, head-down slouching, and excessive
movement between consecutive sampled frames (spec module 8).

Server mode:
    request:  {"cmd": "detect_frame", "frame_path": "/tmp/frame.jpg"}
    response: {
        "posture_present": true,
        "leaning": "none" | "left" | "right",
        "head_down": false,
        "excessive_movement": false,
        "movement_score": 0.012
    }

Note: "excessive_movement" is only meaningful across consecutive samples
of the *same* interview, so this module keeps a small amount of frame-to-
frame state (like speech_to_text.py keeps the loaded Whisper model) —
reset it between interviews with {"cmd": "reset"}.
"""

import argparse

import cv2

from protocol import read_requests, send_response, log
from mediapipe_compat import create_pose_landmarker, to_mp_image

# Pose landmark indices we care about (unchanged — PoseLandmarker still
# returns the same 33-point BlazePose topology as the old mp.solutions.pose).
_NOSE = 0
_LEFT_SHOULDER = 11
_RIGHT_SHOULDER = 12

# Shoulder tilt (as a fraction of shoulder width) beyond which we call it
# "leaning". Head-down is flagged when the nose sits below the shoulder
# line by more than this fraction of shoulder width. Tune during real
# testing with an actual webcam.
_LEAN_THRESHOLD = 0.08
_HEAD_DOWN_THRESHOLD = 0.15
_MOVEMENT_THRESHOLD = 0.06

_pose_landmarker = create_pose_landmarker()

# Frame-to-frame state for excessive-movement detection.
_previous_landmarks = None


def _landmark_xy(landmarks, idx):
    lm = landmarks[idx]
    return lm.x, lm.y


def analyze_posture(frame_bgr) -> dict:
    global _previous_landmarks

    results = _pose_landmarker.detect(to_mp_image(frame_bgr))

    if not results.pose_landmarks:
        return {"posture_present": False, "reason": "no_person_detected"}

    landmarks = results.pose_landmarks[0]

    nose_x, nose_y = _landmark_xy(landmarks, _NOSE)
    left_x, left_y = _landmark_xy(landmarks, _LEFT_SHOULDER)
    right_x, right_y = _landmark_xy(landmarks, _RIGHT_SHOULDER)

    shoulder_mid_x = (left_x + right_x) / 2
    shoulder_mid_y = (left_y + right_y) / 2
    shoulder_width = abs(right_x - left_x) or 1.0

    # Leaning: shoulders tilted, or head shifted sideways relative to them.
    shoulder_tilt = (left_y - right_y) / shoulder_width
    head_offset_x = (nose_x - shoulder_mid_x) / shoulder_width

    leaning = "none"
    combined_lean = shoulder_tilt + head_offset_x
    if combined_lean > _LEAN_THRESHOLD:
        leaning = "right"
    elif combined_lean < -_LEAN_THRESHOLD:
        leaning = "left"

    # Head-down: nose too close to (or below) the shoulder line.
    head_drop = (nose_y - shoulder_mid_y) / shoulder_width
    head_down = head_drop > -_HEAD_DOWN_THRESHOLD

    # Excessive movement: compare key points against the previous frame.
    current_points = [(nose_x, nose_y), (left_x, left_y), (right_x, right_y)]
    movement_score = 0.0
    if _previous_landmarks is not None:
        deltas = [
            ((cx - px) ** 2 + (cy - py) ** 2) ** 0.5
            for (cx, cy), (px, py) in zip(current_points, _previous_landmarks)
        ]
        movement_score = sum(deltas) / len(deltas)
    _previous_landmarks = current_points

    return {
        "posture_present": True,
        "leaning": leaning,
        "head_down": bool(head_down),
        "excessive_movement": movement_score > _MOVEMENT_THRESHOLD,
        "movement_score": round(movement_score, 4),
    }


def run_server():
    global _previous_landmarks
    log("posture_detection.py: server started")
    for request in read_requests():
        cmd = request.get("cmd")
        if cmd == "detect_frame":
            frame = cv2.imread(request.get("frame_path", ""))
            result = {"error": "could not read frame"} if frame is None else analyze_posture(frame)
        elif cmd == "reset":
            _previous_landmarks = None
            result = {"ok": True}
        else:
            result = {"error": f"unknown cmd: {cmd}"}
        send_response(result)


def run_selftest():
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("No webcam found — skipping live capture, but MediaPipe PoseLandmarker loaded OK.")
        return
    for i in range(5):
        ok, frame = cap.read()
        if not ok:
            break
        print(f"frame {i}: {analyze_posture(frame)}")
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

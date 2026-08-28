"""
mediapipe_compat.py — shared helpers for using MediaPipe's current Tasks
API (FaceLandmarker / PoseLandmarker) as a drop-in replacement for the
deprecated mp.solutions.face_mesh / mp.solutions.pose that eye_tracking.py,
emotion_detection.py, and posture_detection.py used to rely on.

Why: recent mediapipe releases (0.10.3x+) raise
    AttributeError: module 'mediapipe' has no attribute 'solutions'
on some Python/platform combinations. MediaPipe's own maintainers have
confirmed the legacy `mp.solutions` API is deprecated and no longer
maintained — pinning an older mediapipe version isn't a reliable fix
either, since old releases mostly predate Python 3.13 wheels. The Tasks
API below is the currently-maintained replacement and works on whatever
recent mediapipe version is installed.

Landmark indices are unchanged: FaceLandmarker still returns 478 points
(including iris, like the old FaceMesh(refine_landmarks=True)), and
PoseLandmarker still returns the same 33-point topology as the old
mp.solutions.pose — so all the existing index-based geometry math in
eye_tracking.py / emotion_detection.py / posture_detection.py keeps working
unchanged; only the setup and result-object access changes.

Model files (.task) are downloaded once to python/models/ and cached there
— this needs an internet connection the first time each script runs.
"""

import os
import urllib.request

import cv2
import mediapipe as mp
from mediapipe.tasks import python as mp_tasks
from mediapipe.tasks.python import vision as mp_vision

_MODELS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "models")

_FACE_LANDMARKER_URL = (
    "https://storage.googleapis.com/mediapipe-models/face_landmarker/"
    "face_landmarker/float16/1/face_landmarker.task"
)
_POSE_LANDMARKER_URL = (
    "https://storage.googleapis.com/mediapipe-models/pose_landmarker/"
    "pose_landmarker_lite/float16/1/pose_landmarker_lite.task"
)


def _ensure_model(url: str, filename: str) -> str:
    os.makedirs(_MODELS_DIR, exist_ok=True)
    path = os.path.join(_MODELS_DIR, filename)
    if not os.path.exists(path):
        urllib.request.urlretrieve(url, path)
    return path


def create_face_landmarker(num_faces: int = 1):
    """FaceLandmarker configured for single-image use — 478 landmarks
    incl. iris, same topology as the old FaceMesh(refine_landmarks=True)."""
    model_path = _ensure_model(_FACE_LANDMARKER_URL, "face_landmarker.task")
    options = mp_vision.FaceLandmarkerOptions(
        base_options=mp_tasks.BaseOptions(model_asset_path=model_path),
        running_mode=mp_vision.RunningMode.IMAGE,
        num_faces=num_faces,
        output_face_blendshapes=True,
    )
    return mp_vision.FaceLandmarker.create_from_options(options)


def create_pose_landmarker():
    """PoseLandmarker configured for single-image use — 33 landmarks,
    same indices as the old mp.solutions.pose (0=nose, 11/12=shoulders...)."""
    model_path = _ensure_model(_POSE_LANDMARKER_URL, "pose_landmarker_lite.task")
    options = mp_vision.PoseLandmarkerOptions(
        base_options=mp_tasks.BaseOptions(model_asset_path=model_path),
        running_mode=mp_vision.RunningMode.IMAGE,
        num_poses=1,
    )
    return mp_vision.PoseLandmarker.create_from_options(options)


def to_mp_image(frame_bgr):
    """Wraps a BGR OpenCV frame as the mp.Image the Tasks API expects."""
    rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
    return mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)

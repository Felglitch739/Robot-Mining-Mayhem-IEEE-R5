# ======================
# AprilTag settings
# ======================
TAG_FAMILY = "tag36h11"
TARGET_TAG_ID = None          # int or None = follow best tag

TAG_SIZE_M = 0.065            # meters (measure your printed tag!)
DESIRED_DIST_M = 0.60         # desired following distance (m)

# ======================
# Camera intrinsics
# (approx defaults; calibrate later)
# ======================
FX = 600.0
FY = 600.0
CX = 320.0
CY = 240.0

FRAME_W = 640
FRAME_H = 480
CAM_INDEX = 0                 # webcam index (VIDEO_PATH overrides)

# ======================
# AprilTag detector tuning
# ======================
QUAD_DECIMATE = 2.0           # ↑ faster, ↓ more accurate
NTHREADS = 2

# ======================
# Control gains
# ======================
KP_DIST = 0.9                # forward/back gain
KP_YAW  = 1.8                # turning gain

MAX_LIN = 0.45               # max linear command
MAX_ANG = 1.25               # max angular command

# ======================
# Lost-tag behavior
# ======================
SEARCH_TURN = 0.6             # rad/s or unitless turn command
STOP_ON_LOST = False          # True = stop instead of searching

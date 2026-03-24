from pupil_apriltags import Detector


class AprilTagVision:
    def __init__(self, tag_family: str, nthreads: int, quad_decimate: float):
        self.detector = Detector(
            families=tag_family,
            nthreads=nthreads,
            quad_decimate=quad_decimate,
            quad_sigma=0.0,
            refine_edges=1,
            decode_sharpening=0.25,
            debug=0
        )

    def detect(self, gray, camera_params, tag_size_m: float):
        fx, fy, cx, cy = camera_params
        detections = self.detector.detect(
            gray,
            estimate_tag_pose=True,
            camera_params=(fx, fy, cx, cy),
            tag_size=tag_size_m
        )
        return detections

    @staticmethod
    def choose_tag(detections, target_id=None):
        if not detections:
            return None
        if target_id is None:
            return max(detections, key=lambda d: d.decision_margin)
        matches = [d for d in detections if d.tag_id == target_id]
        return max(matches, key=lambda d: d.decision_margin) if matches else None

    @staticmethod
    def extract_pose(chosen):
        # pose_t = (x, y, z) in meters in camera frame
        t = chosen.pose_t.flatten()
        return float(t[0]), float(t[1]), float(t[2])  # x, y, z

    @staticmethod
    def extract_center_error(chosen, cx, frame_w):
        # normalized horizontal center error ~ [-1, 1]
        err_px = float(chosen.center[0] - cx)
        return err_px / (frame_w / 2.0)

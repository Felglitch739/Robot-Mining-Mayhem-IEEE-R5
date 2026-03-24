import cv2 #fmllll
import time
import config as C
from vision import AprilTagVision
from controller import FollowController
from io_serial import SerialOutput
# from io_pc import PCOutput
from localization import estimate_robot_pose_from_tag

VIDEO_PATH = None  # or "tag_test.mp4"


def main():
    cap = cv2.VideoCapture(VIDEO_PATH if VIDEO_PATH else C.CAM_INDEX)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, C.FRAME_W)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, C.FRAME_H)

    vision = AprilTagVision(C.TAG_FAMILY, C.NTHREADS, C.QUAD_DECIMATE)

    ctrl = FollowController(
        desired_dist_m=C.DESIRED_DIST_M,
        kp_dist=C.KP_DIST,
        kp_yaw=C.KP_YAW,
        max_lin=C.MAX_LIN,
        max_ang=C.MAX_ANG,
        search_turn=C.SEARCH_TURN,
        stop_on_lost=C.STOP_ON_LOST
    )

    # Windows/test output (no serial)
    # out = PCOutput()

    # Serial output for Pi -> ESP32
    out = SerialOutput("/dev/ttyACM0", 115200)
    # out = SerialOutput("COM5", 115200)

    cam_params = (C.FX, C.FY, C.CX, C.CY)
    last = time.time()

    while True:
        ok, frame = cap.read()
        if not ok:
            print("camera/video ended or read failed or i hope thats what happened fml")
            break

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        dets = vision.detect(gray, cam_params, C.TAG_SIZE_M)
        chosen = vision.choose_tag(dets, C.TARGET_TAG_ID)

        if chosen is not None:
            x, y, z = vision.extract_pose(chosen)
            yaw_err_norm = vision.extract_center_error(chosen, C.CX, C.FRAME_W)

            lin, ang = ctrl.compute(z, yaw_err_norm, has_tag=True)

            # localization from tag
            pose = estimate_robot_pose_from_tag(chosen.tag_id, x, z)

            # draw tag box
            corners = chosen.corners.astype(int)
            for i in range(4):
                p1 = tuple(corners[i])
                p2 = tuple(corners[(i + 1) % 4])
                cv2.line(frame, p1, p2, (0, 255, 0), 2)

            cv2.circle(
                frame,
                (int(chosen.center[0]), int(chosen.center[1])),
                5,
                (255, 0, 0),
                -1
            )

            cv2.putText(
                frame,
                f"ID:{chosen.tag_id} z:{z:.2f}m x:{x:.2f} dm:{chosen.decision_margin:.1f}",
                (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (255, 255, 255),
                2
            )

            cv2.putText(
                frame,
                f"lin:{lin:.2f} ang:{ang:.2f} yawErr:{yaw_err_norm:.2f}",
                (10, 50),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (255, 255, 255),
                2
            )

            if pose is not None:
                robot_x, robot_y = pose
                cv2.putText(
                    frame,
                    f"RobotX:{robot_x:.2f} RobotY:{robot_y:.2f}",
                    (10, 75),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.6,
                    (255, 255, 0),
                    2
                )

        else:
            lin, ang = ctrl.compute(0.0, 0.0, has_tag=False)
            cv2.putText(
                frame,
                "No tag",
                (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 0, 255),
                2
            )

        out.send(lin, ang)

        now = time.time()
        fps = 1.0 / (now - last) if now != last else 0.0
        last = now

        cv2.putText(
            frame,
            f"FPS:{fps:.1f}",
            (10, C.FRAME_H - 15),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (255, 255, 255),
            2
        )

        cv2.imshow("R5 AprilTag Bot", frame)
        if (cv2.waitKey(1) & 0xFF) == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
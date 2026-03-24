from tag_map import TAG_MAP

def estimate_robot_pose_from_tag(tag_id, cam_x, cam_z):
    if tag_id not in TAG_MAP:
        return None

    tag = TAG_MAP[tag_id]

    # super simple first-pass estimate will need to be improved with better geometry and yaw handling
    robot_x = tag["x"] - cam_z
    robot_y = tag["y"] - cam_x

    return robot_x, robot_y
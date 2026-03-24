def clamp(x, lo, hi):
    return max(lo, min(hi, x))


class FollowController:
    def __init__(
        self,
        desired_dist_m,
        kp_dist,
        kp_yaw,
        max_lin,
        max_ang,
        search_turn,
        stop_on_lost,
    ):
        self.desired = desired_dist_m
        self.kp_dist = kp_dist
        self.kp_yaw = kp_yaw
        self.max_lin = max_lin
        self.max_ang = max_ang
        self.search_turn = search_turn
        self.stop_on_lost = stop_on_lost

    def compute(self, z_dist_m, yaw_err_norm, has_tag: bool):

        if not has_tag:
            if self.stop_on_lost:
                return 0.0, 0.0
            else:
                # search behavior: slow turn to reacquire tag
                return 0.0, self.search_turn

        # distance error
        dist_err = z_dist_m - self.desired

        # proportional control
        lin = clamp(self.kp_dist * dist_err, -self.max_lin, self.max_lin)
        ang = clamp(-self.kp_yaw * yaw_err_norm, -self.max_ang, self.max_ang)

        return lin, ang
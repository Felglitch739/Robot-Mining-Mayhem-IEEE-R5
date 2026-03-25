from __future__ import annotations

import argparse
import random
import time
from dataclasses import dataclass
from typing import Optional

import serial


@dataclass
class CollectionConfig:
    speed: int = 170
    obstacle_threshold_cm: float = 22.0
    loop_delay_s: float = 0.05

    sweep_interval_s: float = 4.0
    sweep_turn_s: float = 0.35

    reverse_time_s: float = 0.65
    stop_settle_s: float = 0.15
    min_turn_s: float = 0.45
    max_turn_s: float = 1.20


@dataclass
class Telemetry:
    yaw_deg: float
    front_cm: float
    side_cm: float
    started: bool


class PicoProtocol:
    """Serial wrapper for the Pico command protocol.

    Supported commands on Pico side:
        F, B, L, R, S, M1, M0, SPEED <0-255>, DIST, DATA, START
    """

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 0.2) -> None:
        self.serial_port = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(2.0)
        self.flush_input()

    def close(self) -> None:
        if self.serial_port.is_open:
            self.serial_port.close()

    def flush_input(self) -> None:
        self.serial_port.reset_input_buffer()

    def send(self, command: str) -> None:
        self.serial_port.write(f"{command.strip()}\n".encode("utf-8"))

    def read_line(self) -> str:
        return self.serial_port.readline().decode("utf-8", errors="ignore").strip()

    def request_distance(self, max_reads: int = 8) -> Optional[float]:
        self.send("DIST")
        for _ in range(max_reads):
            line = self.read_line()
            if not line or not line.startswith("DIST:"):
                continue
            value_text = line.split(":", maxsplit=1)[1]
            try:
                return float(value_text)
            except ValueError:
                return None
        return None

    def request_data(self, max_reads: int = 12) -> Optional[Telemetry]:
        self.send("DATA")
        for _ in range(max_reads):
            line = self.read_line()
            if not line or not line.startswith("DATA:"):
                continue
            payload = line.split(":", maxsplit=1)[1]
            parts = [token.strip() for token in payload.split(",")]
            if len(parts) != 4:
                return None
            try:
                yaw = float(parts[0])
                front = float(parts[1])
                side = float(parts[2])
                started = bool(int(parts[3]))
                return Telemetry(yaw_deg=yaw, front_cm=front, side_cm=side, started=started)
            except ValueError:
                return None
        return None

    def set_speed(self, speed: int) -> None:
        if not 0 <= speed <= 255:
            raise ValueError("Speed must be between 0 and 255")
        self.send(f"SPEED {speed}")

    def forward(self) -> None:
        self.send("F")

    def backward(self) -> None:
        self.send("B")

    def left(self) -> None:
        self.send("L")

    def right(self) -> None:
        self.send("R")

    def stop(self) -> None:
        self.send("S")

    def minerals_on(self) -> None:
        self.send("M1")

    def minerals_off(self) -> None:
        self.send("M0")

    def start_robot(self) -> None:
        self.send("START")


class MineralCollector:
    def __init__(self, robot: PicoProtocol, config: CollectionConfig) -> None:
        self.robot = robot
        self.config = config
        self.last_sweep_time = time.monotonic()
        self.sweep_left = True

    def startup(self) -> None:
        self.robot.start_robot()
        self.robot.stop()
        time.sleep(self.config.stop_settle_s)
        self.robot.set_speed(self.config.speed)
        self.robot.minerals_on()
        self.robot.forward()
        print("Collection mode started.")

    def avoid_obstacle(self, distance_cm: float) -> None:
        print(f"Obstacle at {distance_cm:.2f} cm -> avoidance maneuver")

        self.robot.stop()
        time.sleep(self.config.stop_settle_s)

        self.robot.backward()
        time.sleep(self.config.reverse_time_s)

        turn_time = random.uniform(self.config.min_turn_s, self.config.max_turn_s)
        if random.random() < 0.5:
            self.robot.left()
            direction = "left"
        else:
            self.robot.right()
            direction = "right"

        print(f"Turning {direction} for {turn_time:.2f} s")
        time.sleep(turn_time)

        self.robot.stop()
        time.sleep(0.08)
        self.robot.forward()

    def do_coverage_sweep(self) -> None:
        now = time.monotonic()
        if now - self.last_sweep_time < self.config.sweep_interval_s:
            return

        if self.sweep_left:
            self.robot.left()
            direction = "left"
        else:
            self.robot.right()
            direction = "right"

        print(f"Coverage sweep: {direction}")
        time.sleep(self.config.sweep_turn_s)
        self.robot.forward()

        self.sweep_left = not self.sweep_left
        self.last_sweep_time = now

    def run(self) -> None:
        self.startup()
        while True:
            telemetry = self.robot.request_data()

            distance: Optional[float]
            if telemetry is not None:
                distance = telemetry.front_cm
                print(
                    f"Yaw: {telemetry.yaw_deg:6.2f} | "
                    f"Front: {telemetry.front_cm:6.2f} cm | "
                    f"Side: {telemetry.side_cm:6.2f} cm | "
                    f"Started: {int(telemetry.started)}"
                )
            else:
                distance = self.robot.request_distance()
                if distance is not None:
                    print(f"Distance (fallback): {distance:.2f} cm")

            if distance is not None and 0 < distance < self.config.obstacle_threshold_cm:
                self.avoid_obstacle(distance)
            else:
                self.do_coverage_sweep()
                self.robot.forward()

            time.sleep(self.config.loop_delay_s)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Best-case mineral collection controller for Pico")
    parser.add_argument("--port", default="/dev/serial0", help="Serial device path (e.g. /dev/serial0, /dev/ttyUSB0)")
    parser.add_argument("--baudrate", type=int, default=115200, help="Serial baudrate")
    parser.add_argument("--speed", type=int, default=170, help="Motor speed (0-255)")
    parser.add_argument("--threshold", type=float, default=22.0, help="Obstacle threshold in cm")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    config = CollectionConfig(
        speed=args.speed,
        obstacle_threshold_cm=args.threshold,
    )

    try:
        robot = PicoProtocol(port=args.port, baudrate=args.baudrate)
    except serial.SerialException as exc:
        print(f"Failed to open serial connection: {exc}")
        return

    collector = MineralCollector(robot, config)

    try:
        collector.run()
    except KeyboardInterrupt:
        print("\nStopping collection mode...")
    finally:
        robot.stop()
        robot.minerals_off()
        robot.close()


if __name__ == "__main__":
    main()

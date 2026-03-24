import serial


class SerialOutput:
    def __init__(self, port: str, baud: int = 115200):
        self.ser = serial.Serial(port, baud, timeout=0.02)

    def send(self, lin, ang):
        msg = f"{lin:.3f},{ang:.3f}\n"
        self.ser.write(msg.encode("utf-8"))
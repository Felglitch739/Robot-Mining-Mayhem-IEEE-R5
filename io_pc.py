import time 
class PCOutput:
    def __init__(self):
        self.t0=time.time()
    def send(self,lin,ang):
        print(f"({lin:.3f},{ang:.3f})")
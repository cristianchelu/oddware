#!/usr/bin/env python3
"""Long-running UART capture with USB reconnect. See tools/uart-console.sh."""
import signal
import sys
import time

import serial
from serial import SerialException

TTY = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
LOG = sys.argv[3] if len(sys.argv) > 3 else "tools/ota/logs/uart-session.log"

running = True


def stop(_sig, _frame):
    global running
    running = False


signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT, stop)

print(f"UART grabber: {TTY} @ {BAUD} -> {LOG}", flush=True)
print("Power on / reproduce. Say when to stop.", flush=True)

out = open(LOG, "ab", buffering=0)
ser = None

try:
    while running:
        try:
            if ser is None or not ser.is_open:
                ser = serial.Serial(TTY, BAUD, timeout=0.5)
                print(f"opened {TTY}", flush=True)
            data = ser.read(8192)
            if data:
                out.write(data)
                out.flush()
        except SerialException as exc:
            print(f"serial error ({exc}); retry in 1s", flush=True)
            if ser is not None:
                try:
                    ser.close()
                except SerialException:
                    pass
                ser = None
            time.sleep(1.0)
finally:
    if ser is not None:
        ser.close()
    out.close()
    print(f"UART grabber stopped. log={LOG}", flush=True)

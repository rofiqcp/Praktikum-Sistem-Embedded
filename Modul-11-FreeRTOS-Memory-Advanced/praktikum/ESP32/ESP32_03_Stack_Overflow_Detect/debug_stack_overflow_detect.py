#!/usr/bin/env python3
"""
Debug script for ESP32_03_Stack_Overflow_Detect
Captures stack high-water marks, detects overflow events.
Usage: python debug_stack_overflow_detect.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Stack Overflow Detection Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"stack_overflow_log_{ts}.csv"

    hwm_pat = re.compile(r"\[(\w+)\].*HWM[=:]\s*(\d+)")
    overflow_pat = re.compile(r"STACK OVERFLOW.*task:\s*\"(\w+)\"")

    records = []
    overflows = []

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "elapsed_s", "task", "hwm_words", "event"])

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)
                elapsed = round(time.time() - start, 2)

                m = hwm_pat.search(line)
                if m:
                    task_name = m.group(1)
                    hwm = int(m.group(2))
                    w.writerow([datetime.now().isoformat(), elapsed, task_name, hwm, "hwm"])
                    records.append((task_name, hwm))

                m = overflow_pat.search(line)
                if m:
                    task_name = m.group(1)
                    w.writerow([datetime.now().isoformat(), elapsed, task_name, 0, "OVERFLOW"])
                    overflows.append(task_name)
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("STACK OVERFLOW DETECTION SUMMARY")
    print("=" * 60)
    tasks = set(r[0] for r in records)
    for t in sorted(tasks):
        hwms = [r[1] for r in records if r[0] == t]
        print(f"  Task '{t}': HWM range {min(hwms)}-{max(hwms)} words ({len(hwms)} samples)")
    if overflows:
        print(f"\n  *** OVERFLOWS DETECTED: {overflows}")
    else:
        print(f"\n  No overflow detected during capture.")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

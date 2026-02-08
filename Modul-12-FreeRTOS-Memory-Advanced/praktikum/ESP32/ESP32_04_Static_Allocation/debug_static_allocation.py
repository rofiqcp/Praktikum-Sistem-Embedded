#!/usr/bin/env python3
"""
Debug script for ESP32_04_Static_Allocation
Monitors LED blink, queue activity, and heap delta from static allocation.
Usage: python debug_static_allocation.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Static Allocation Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"static_alloc_log_{ts}.csv"

    blink_pat = re.compile(r"\[BLINK\] LED (\w+)\s+count=(\d+)\s+HWM=(\d+)")
    heap_pat  = re.compile(r"Free heap:\s*(\d+)")
    delta_pat = re.compile(r"Heap delta.*:\s*(-?\d+)")

    records = []
    heap_delta = None

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "elapsed_s", "led_state", "count", "hwm", "free_heap"])
            current_heap = ""

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = heap_pat.search(line)
                if m:
                    current_heap = m.group(1)

                m = delta_pat.search(line)
                if m:
                    heap_delta = int(m.group(1))

                m = blink_pat.search(line)
                if m:
                    elapsed = round(time.time() - start, 2)
                    w.writerow([datetime.now().isoformat(), elapsed,
                                m.group(1), int(m.group(2)), int(m.group(3)),
                                current_heap])
                    records.append({"led": m.group(1), "count": int(m.group(2)),
                                    "hwm": int(m.group(3))})
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("STATIC ALLOCATION SUMMARY")
    print("=" * 60)
    if records:
        print(f"  Blink events    : {len(records)}")
        hwms = [r['hwm'] for r in records]
        print(f"  Blink HWM range : {min(hwms)} – {max(hwms)} words")
    if heap_delta is not None:
        status = "✓ PASS (truly static)" if abs(heap_delta) < 100 else "✗ FAIL (heap used!)"
        print(f"  Heap delta      : {heap_delta} bytes → {status}")
    print(f"  CSV log         : {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

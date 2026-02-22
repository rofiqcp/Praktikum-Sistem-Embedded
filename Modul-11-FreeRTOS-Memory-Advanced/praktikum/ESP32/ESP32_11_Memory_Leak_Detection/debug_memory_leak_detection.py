#!/usr/bin/env python3
"""
Debug script for ESP32_11_Memory_Leak_Detection
Tracks heap free over time, detects leak trend, plots if matplotlib available.
Usage: python debug_memory_leak_detection.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Memory Leak Detection Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=120)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"leak_detect_log_{ts}.csv"

    mon_pat = re.compile(
        r"\[MON\]\s+t=(\d+)\s*ms\s+free=(\d+)\s+min_ever=(\d+)\s+largest=(\d+)\s+alloc_blocks=(\d+)"
    )
    leak_pat = re.compile(r"\[LEAK\].*total leaked:\s*(\d+)\s*bytes,\s*(\d+)\s*blocks")
    detect_pat = re.compile(r"MEMORY LEAK DETECTED")
    delta_pat = re.compile(r"Heap delta:\s*([+-]?\d+)")

    records = []
    leak_detected = False
    total_leaked = 0

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "tick_ms", "free_heap", "min_ever",
                         "largest_block", "alloc_blocks", "delta", "leaked_total"])

            cur_delta = ""
            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = delta_pat.search(line)
                if m:
                    cur_delta = m.group(1)

                m = mon_pat.search(line)
                if m:
                    row = [datetime.now().isoformat(),
                           int(m.group(1)), int(m.group(2)),
                           int(m.group(3)), int(m.group(4)),
                           int(m.group(5)), cur_delta, total_leaked]
                    w.writerow(row)
                    records.append({"tick": int(m.group(1)), "free": int(m.group(2)),
                                    "alloc_blocks": int(m.group(5))})
                    cur_delta = ""

                m = leak_pat.search(line)
                if m:
                    total_leaked = int(m.group(1))

                if detect_pat.search(line):
                    leak_detected = True
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("MEMORY LEAK DETECTION SUMMARY")
    print("=" * 60)
    if records:
        frees = [r["free"] for r in records]
        print(f"  Samples          : {len(records)}")
        print(f"  Free heap start  : {frees[0]} bytes")
        print(f"  Free heap end    : {frees[-1]} bytes")
        total_drop = frees[0] - frees[-1]
        duration_s = (records[-1]["tick"] - records[0]["tick"]) / 1000.0
        print(f"  Total decrease   : {total_drop} bytes over {duration_s:.1f}s")
        if duration_s > 0:
            print(f"  Leak rate        : {total_drop / duration_s:.1f} bytes/s")
        print(f"  Leak detected    : {'YES ⚠️' if leak_detected else 'NO ✓'}")
        print(f"  Total leaked     : {total_leaked} bytes")
    else:
        print("  No data captured.")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Debug script for ESP32_01_Heap_Monitor
Captures heap statistics via serial, logs to CSV, prints summary.
Usage: python debug_heap_monitor.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from collections import defaultdict
from datetime import datetime


def parse_args():
    parser = argparse.ArgumentParser(description="Heap Monitor Debug Logger")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--duration", type=int, default=60, help="Capture duration (s)")
    return parser.parse_args()


def main():
    args = parse_args()
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"heap_monitor_log_{timestamp}.csv"

    patterns = {
        "free_heap": re.compile(r"Free heap\s*:\s*(\d+)"),
        "min_ever": re.compile(r"Min-ever free heap\s*:\s*(\d+)"),
        "default_free": re.compile(r"DEFAULT\s+free\s*:\s*(\d+)"),
        "internal_free": re.compile(r"INTERNAL\s+free\s*:\s*(\d+)"),
        "largest_block": re.compile(r"Largest free block\s*:\s*(\d+)"),
        "total_alloc": re.compile(r"Total allocated bytes\s*:\s*(\d+)"),
    }

    records = []
    current = {}

    print(f"[*] Connecting to {args.port} @ {args.baud} baud")
    print(f"[*] Logging to {csv_file} for {args.duration}s")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open serial port: {e}")
        sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["timestamp", "elapsed_s", "free_heap", "min_ever",
                             "default_free", "internal_free", "largest_block", "total_alloc"])

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                try:
                    line = raw.decode("utf-8", errors="replace").strip()
                except Exception:
                    continue
                if not line:
                    continue
                print(line)

                for key, pat in patterns.items():
                    m = pat.search(line)
                    if m:
                        current[key] = int(m.group(1))

                if "========" in line and len(current) >= 3:
                    elapsed = round(time.time() - start, 2)
                    row = [
                        datetime.now().isoformat(),
                        elapsed,
                        current.get("free_heap", ""),
                        current.get("min_ever", ""),
                        current.get("default_free", ""),
                        current.get("internal_free", ""),
                        current.get("largest_block", ""),
                        current.get("total_alloc", ""),
                    ]
                    writer.writerow(row)
                    records.append(current.copy())
                    current = {}

    except KeyboardInterrupt:
        print("\n[*] Interrupted by user")
    finally:
        ser.close()

    # Summary
    print("\n" + "=" * 60)
    print("HEAP MONITOR SUMMARY")
    print("=" * 60)
    if records:
        free_vals = [r["free_heap"] for r in records if "free_heap" in r]
        if free_vals:
            print(f"  Samples         : {len(free_vals)}")
            print(f"  Free heap range : {min(free_vals)} – {max(free_vals)} bytes")
            print(f"  Free heap avg   : {sum(free_vals)//len(free_vals)} bytes")
            delta = free_vals[0] - free_vals[-1]
            print(f"  Net change      : {delta:+d} bytes ({'leak?' if delta > 0 else 'ok'})")
        min_evers = [r["min_ever"] for r in records if "min_ever" in r]
        if min_evers:
            print(f"  Min-ever seen   : {min(min_evers)} bytes")
    else:
        print("  No data captured.")
    print(f"  CSV log         : {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

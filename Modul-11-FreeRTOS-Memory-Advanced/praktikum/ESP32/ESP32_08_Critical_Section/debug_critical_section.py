#!/usr/bin/env python3
"""
Debug script for ESP32_08_Critical_Section
Captures critical section vs mutex timing comparison.
Usage: python debug_critical_section.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Critical Section Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"critical_section_log_{ts}.csv"

    crit_pat = re.compile(r"\[CRIT T(\d+)\] counter=(\d+)\s+crit_time=(\d+)")
    mutex_pat = re.compile(r"\[MUTEX T(\d+)\] counter=(\d+)\s+mutex_time=(\d+)")
    summary_crit_pat = re.compile(r"\[CRITICAL\].*ops=(\d+)\s+avg=(\d+)")
    summary_mutex_pat = re.compile(r"\[MUTEX\].*ops=(\d+)\s+avg=(\d+)")

    crit_times = []
    mutex_times = []

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "method", "task_id", "counter", "time_us"])

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = crit_pat.search(line)
                if m:
                    t_us = int(m.group(3))
                    crit_times.append(t_us)
                    w.writerow([datetime.now().isoformat(), "CRITICAL",
                                m.group(1), m.group(2), t_us])

                m = mutex_pat.search(line)
                if m:
                    t_us = int(m.group(3))
                    mutex_times.append(t_us)
                    w.writerow([datetime.now().isoformat(), "MUTEX",
                                m.group(1), m.group(2), t_us])
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("CRITICAL SECTION vs MUTEX SUMMARY")
    print("=" * 60)
    for name, data in [("CRITICAL", crit_times), ("MUTEX", mutex_times)]:
        if data:
            avg = sum(data) // len(data)
            print(f"  {name:10s}: {len(data)} samples, avg={avg} us, "
                  f"min={min(data)} us, max={max(data)} us")
        else:
            print(f"  {name:10s}: no data")
    if crit_times and mutex_times:
        ca = sum(crit_times) // len(crit_times)
        ma = sum(mutex_times) // len(mutex_times)
        print(f"\n  Critical is {'faster' if ca < ma else 'slower'} than mutex by "
              f"{abs(ca - ma)} us on average")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Debug script for ESP32_09_Heap_Fragmentation
Tracks fragmentation percentage across phases and rounds.
Usage: python debug_heap_fragmentation.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Heap Fragmentation Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=120)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"heap_frag_log_{ts}.csv"

    phase_pat = re.compile(r"\[(PHASE \d.*?|CLEANUP.*?)\]")
    free_pat = re.compile(r"Total free\s*:\s*(\d+)")
    largest_pat = re.compile(r"Largest block\s*:\s*(\d+)")
    frag_pat = re.compile(r"Fragmentation\s*:\s*([\d.]+)")
    result_pat = re.compile(r"RESULT:\s*(SUCCESS|FAILED)")
    round_pat = re.compile(r"ROUND\s+(\d+)")

    records = []
    current = {}

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "round", "phase", "total_free",
                         "largest_block", "frag_pct", "large_alloc_result"])

            cur_round = 0
            cur_phase = ""
            cur_result = ""

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = round_pat.search(line)
                if m:
                    cur_round = int(m.group(1))

                m = phase_pat.search(line)
                if m:
                    if current.get("phase"):
                        w.writerow([datetime.now().isoformat(), cur_round,
                                    current.get("phase", ""),
                                    current.get("free", ""),
                                    current.get("largest", ""),
                                    current.get("frag", ""),
                                    cur_result])
                        records.append(current.copy())
                        cur_result = ""
                    current = {"phase": m.group(1)}

                m = free_pat.search(line)
                if m:
                    current["free"] = int(m.group(1))

                m = largest_pat.search(line)
                if m:
                    current["largest"] = int(m.group(1))

                m = frag_pat.search(line)
                if m:
                    current["frag"] = float(m.group(1))

                m = result_pat.search(line)
                if m:
                    cur_result = m.group(1)
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("HEAP FRAGMENTATION SUMMARY")
    print("=" * 60)
    if records:
        frags = [r["frag"] for r in records if "frag" in r]
        if frags:
            print(f"  Samples          : {len(frags)}")
            print(f"  Frag range       : {min(frags):.1f}% – {max(frags):.1f}%")
            print(f"  Max fragmentation: {max(frags):.1f}%")
    else:
        print("  No data captured.")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

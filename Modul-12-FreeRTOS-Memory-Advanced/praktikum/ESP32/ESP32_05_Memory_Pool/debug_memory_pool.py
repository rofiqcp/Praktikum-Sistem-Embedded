#!/usr/bin/env python3
"""
Debug script for ESP32_05_Memory_Pool
Captures pool vs dynamic allocation benchmarks.
Usage: python debug_memory_pool.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Memory Pool Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"memory_pool_log_{ts}.csv"

    bench_pat = re.compile(
        r"\[(POOL|DYNAMIC)\]\s+alloc=(\d+)\s*us\s+free=(\d+)\s*us\s+success=(\d+)/(\d+)"
    )
    pool_avail_pat = re.compile(r"Available blocks:\s*(\d+)")
    frag_pat = re.compile(r"Frag=([\d.]+)%")

    records = []

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "method", "alloc_us", "free_us",
                         "success", "total", "frag_pct", "pool_avail"])

            cur_frag = ""
            cur_pool_avail = ""

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = frag_pat.search(line)
                if m:
                    cur_frag = m.group(1)

                m = pool_avail_pat.search(line)
                if m:
                    cur_pool_avail = m.group(1)

                m = bench_pat.search(line)
                if m:
                    row = [datetime.now().isoformat(), m.group(1),
                           int(m.group(2)), int(m.group(3)),
                           int(m.group(4)), int(m.group(5)),
                           cur_frag, cur_pool_avail]
                    w.writerow(row)
                    records.append(row)
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("MEMORY POOL SUMMARY")
    print("=" * 60)
    for method in ["POOL", "DYNAMIC"]:
        subset = [r for r in records if r[1] == method]
        if subset:
            avg_alloc = sum(r[2] for r in subset) // len(subset)
            avg_free = sum(r[3] for r in subset) // len(subset)
            print(f"  {method:8s}: avg alloc={avg_alloc} us, avg free={avg_free} us ({len(subset)} runs)")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

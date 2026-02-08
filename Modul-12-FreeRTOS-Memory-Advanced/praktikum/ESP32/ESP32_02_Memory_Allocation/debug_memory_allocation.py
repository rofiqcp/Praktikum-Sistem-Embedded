#!/usr/bin/env python3
"""
Debug script for ESP32_02_Memory_Allocation
Captures allocation benchmarks and fragmentation data via serial.
Usage: python debug_memory_allocation.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Memory Allocation Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_bench = f"mem_alloc_bench_{ts}.csv"
    csv_frag  = f"mem_alloc_frag_{ts}.csv"

    bench_pat = re.compile(
        r"(\w[\w_]*)\((\d+)\)\s+alloc=(\d+)\s*us\s+free=(\d+)\s*us\s+total=(\d+)\s*us"
    )
    heap_pat = re.compile(
        r"\[(\w+)\]\s*Free:\s*(\d+)\s+Min-ever:\s*(\d+)\s+Largest-blk:\s*(\d+)"
    )

    bench_rows = []
    frag_rows  = []

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        while time.time() - start < args.duration:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            print(line)

            m = bench_pat.search(line)
            if m:
                bench_rows.append([datetime.now().isoformat(),
                                   m.group(1), int(m.group(2)),
                                   int(m.group(3)), int(m.group(4)), int(m.group(5))])

            m = heap_pat.search(line)
            if m:
                frag_rows.append([datetime.now().isoformat(),
                                  m.group(1), int(m.group(2)),
                                  int(m.group(3)), int(m.group(4))])
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    # Write CSVs
    with open(csv_bench, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["timestamp", "method", "size", "alloc_us", "free_us", "total_us"])
        w.writerows(bench_rows)

    with open(csv_frag, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["timestamp", "phase", "free", "min_ever", "largest_block"])
        w.writerows(frag_rows)

    print("\n" + "=" * 60)
    print("MEMORY ALLOCATION SUMMARY")
    print("=" * 60)
    if bench_rows:
        for method in set(r[1] for r in bench_rows):
            subset = [r for r in bench_rows if r[1] == method]
            avg_alloc = sum(r[3] for r in subset) // len(subset)
            avg_free  = sum(r[4] for r in subset) // len(subset)
            print(f"  {method}: avg alloc={avg_alloc} us, avg free={avg_free} us ({len(subset)} samples)")
    if frag_rows:
        print(f"\n  Heap snapshots: {len(frag_rows)}")
        frees = [r[2] for r in frag_rows]
        print(f"  Free heap range: {min(frees)} – {max(frees)}")
    print(f"  Benchmark CSV : {csv_bench}")
    print(f"  Fragment CSV  : {csv_frag}")
    print("=" * 60)


if __name__ == "__main__":
    main()

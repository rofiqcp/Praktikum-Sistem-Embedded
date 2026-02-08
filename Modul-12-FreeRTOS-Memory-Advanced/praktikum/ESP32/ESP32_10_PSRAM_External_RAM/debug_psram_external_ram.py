#!/usr/bin/env python3
"""
Debug script for ESP32_10_PSRAM_External_RAM
Captures memory region info and DRAM vs SPIRAM speed benchmarks.
Usage: python debug_psram_external_ram.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="PSRAM External RAM Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"psram_log_{ts}.csv"

    bench_pat = re.compile(
        r"\[BENCH\]\s+(\w+)\s+(\d+)\s*B:\s+write=(\d+)\s*us\s+\(([\d.]+)\s*MB/s\)\s+"
        r"read=(\d+)\s*us\s+\(([\d.]+)\s*MB/s\)"
    )
    region_pat = re.compile(r"\[(\w+.*?)\]")
    free_pat = re.compile(r"Free\s*:\s*(\d+)")

    bench_records = []

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "region", "size", "write_us", "write_mbps",
                         "read_us", "read_mbps"])

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
                    row = [datetime.now().isoformat(),
                           m.group(1).strip(), int(m.group(2)),
                           int(m.group(3)), float(m.group(4)),
                           int(m.group(5)), float(m.group(6))]
                    w.writerow(row)
                    bench_records.append(row)
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("PSRAM / EXTERNAL RAM SUMMARY")
    print("=" * 60)
    for region in ["DRAM", "SPIRAM"]:
        subset = [r for r in bench_records if r[1] == region]
        if subset:
            avg_w = sum(r[4] for r in subset) / len(subset)
            avg_r = sum(r[6] for r in subset) / len(subset)
            print(f"  {region:8s}: avg write={avg_w:.2f} MB/s, avg read={avg_r:.2f} MB/s "
                  f"({len(subset)} tests)")
        else:
            print(f"  {region:8s}: no successful benchmarks (hardware not available?)")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

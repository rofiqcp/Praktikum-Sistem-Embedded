#!/usr/bin/env python3
"""
Debug script for ESP32_12_System_Dashboard
Captures comprehensive system dashboard output: tasks, heap, CPU, uptime.
Usage: python debug_system_dashboard.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime
from collections import defaultdict


def parse_args():
    p = argparse.ArgumentParser(description="System Dashboard Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=120)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_heap = f"dashboard_heap_{ts}.csv"
    csv_tasks = f"dashboard_tasks_{ts}.csv"

    uptime_pat = re.compile(r"Uptime:\s*(\d+:\d+:\d+)\s*\((\d+)\s*ms\)")
    heap_free_pat = re.compile(r"Free:\s*(\d+)\s*B\s+Used:\s*([\d.]+)%")
    heap_min_pat = re.compile(r"Min-ever free:\s*(\d+)")
    frag_pat = re.compile(r"Fragmentation:\s*([\d.]+)%")
    largest_pat = re.compile(r"Largest block:\s*(\d+)")
    tasks_pat = re.compile(r"Active tasks:\s*(\d+)")
    heartbeat_pat = re.compile(r"Heartbeats:\s*(\d+)")
    proc_pat = re.compile(r"\[PROC\]\s+(\d+) readings\s+avg_temp=([\d.]+)")

    heap_records = []
    task_count_records = []
    current = {}

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_heap, "w", newline="") as fh, \
             open(csv_tasks, "w", newline="") as ft:
            wh = csv.writer(fh)
            wh.writerow(["timestamp", "uptime_ms", "free_heap", "used_pct",
                          "min_ever", "largest_block", "frag_pct",
                          "heartbeats", "active_tasks"])
            wt = csv.writer(ft)
            wt.writerow(["timestamp", "raw_line"])

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = uptime_pat.search(line)
                if m:
                    current["uptime"] = int(m.group(2))

                m = heartbeat_pat.search(line)
                if m:
                    current["heartbeats"] = int(m.group(1))

                m = heap_free_pat.search(line)
                if m:
                    current["free"] = int(m.group(1))
                    current["used_pct"] = float(m.group(2))

                m = heap_min_pat.search(line)
                if m:
                    current["min_ever"] = int(m.group(1))

                m = largest_pat.search(line)
                if m:
                    current["largest"] = int(m.group(1))

                m = frag_pat.search(line)
                if m:
                    current["frag"] = float(m.group(1))

                m = tasks_pat.search(line)
                if m:
                    current["tasks"] = int(m.group(1))
                    # Write complete snapshot
                    wh.writerow([
                        datetime.now().isoformat(),
                        current.get("uptime", ""),
                        current.get("free", ""),
                        current.get("used_pct", ""),
                        current.get("min_ever", ""),
                        current.get("largest", ""),
                        current.get("frag", ""),
                        current.get("heartbeats", ""),
                        current.get("tasks", ""),
                    ])
                    heap_records.append(current.copy())
                    current = {}

                # Log task list lines
                if "║" in line and any(s in line for s in ["R ", "B ", "S ", "X "]):
                    wt.writerow([datetime.now().isoformat(), line])
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("SYSTEM DASHBOARD SUMMARY")
    print("=" * 60)
    if heap_records:
        frees = [r["free"] for r in heap_records if "free" in r]
        frags = [r["frag"] for r in heap_records if "frag" in r]
        print(f"  Dashboard snapshots: {len(heap_records)}")
        if frees:
            print(f"  Free heap range    : {min(frees)} – {max(frees)} bytes")
        if frags:
            print(f"  Fragmentation range: {min(frags):.1f}% – {max(frags):.1f}%")
        if "tasks" in heap_records[-1]:
            print(f"  Active tasks (last): {heap_records[-1]['tasks']}")
        if "heartbeats" in heap_records[-1]:
            print(f"  Heartbeats (last)  : {heap_records[-1]['heartbeats']}")
    else:
        print("  No dashboard data captured.")
    print(f"  Heap CSV : {csv_heap}")
    print(f"  Tasks CSV: {csv_tasks}")
    print("=" * 60)


if __name__ == "__main__":
    main()

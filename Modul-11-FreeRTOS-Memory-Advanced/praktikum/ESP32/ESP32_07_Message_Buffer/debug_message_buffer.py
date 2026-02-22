#!/usr/bin/env python3
"""
Debug script for ESP32_07_Message_Buffer
Tracks message types, counts, and buffer utilization.
Usage: python debug_message_buffer.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from collections import Counter
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Message Buffer Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"message_buffer_log_{ts}.csv"

    cons_pat = re.compile(r"\[CONS\]\s+#(\d+)\s+(\w+)")
    space_pat = re.compile(r"space available:\s*(\d+)")
    sent_pat = re.compile(r"\[(SENSOR|EVENT|CMD)\]\s+Sent.*bytes=(\d+)")

    msg_types = Counter()
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
            w.writerow(["timestamp", "direction", "msg_type", "msg_num", "bytes", "space_avail"])

            cur_space = ""
            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = space_pat.search(line)
                if m:
                    cur_space = m.group(1)

                m = cons_pat.search(line)
                if m:
                    msg_types[m.group(2)] += 1
                    w.writerow([datetime.now().isoformat(), "RECV",
                                m.group(2), m.group(1), "", cur_space])

                m = sent_pat.search(line)
                if m:
                    w.writerow([datetime.now().isoformat(), "SEND",
                                m.group(1), "", m.group(2), ""])
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("MESSAGE BUFFER SUMMARY")
    print("=" * 60)
    total = sum(msg_types.values())
    print(f"  Total messages consumed: {total}")
    for mtype, cnt in msg_types.most_common():
        print(f"    {mtype:10s}: {cnt}")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

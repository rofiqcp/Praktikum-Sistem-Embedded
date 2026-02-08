#!/usr/bin/env python3
"""
Debug script for ESP32_06_Stream_Buffer
Logs producer/consumer byte flow and buffer utilization.
Usage: python debug_stream_buffer.py [--port PORT] [--baud BAUD] [--duration SEC]
"""

import serial
import argparse
import time
import csv
import re
import sys
from datetime import datetime


def parse_args():
    p = argparse.ArgumentParser(description="Stream Buffer Debug Logger")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=int, default=60)
    return p.parse_args()


def main():
    args = parse_args()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"stream_buffer_log_{ts}.csv"

    prod_pat = re.compile(r"\[PROD\] Sent (\d+)/(\d+).*buf_used=(\d+)\s+buf_free=(\d+)")
    cons_pat = re.compile(r"\[CONS\] Received (\d+) bytes")
    stat_pat = re.compile(r"\[STATS\] Stream: used=(\d+)\s+free=(\d+)\s+full=(\d+)\s+empty=(\d+)")

    records = []
    total_sent = 0
    total_recv = 0

    print(f"[*] Connecting {args.port} @ {args.baud}")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERROR] {e}"); sys.exit(1)

    start = time.time()
    try:
        with open(csv_file, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "event", "bytes", "buf_used", "buf_free"])

            while time.time() - start < args.duration:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                print(line)

                m = prod_pat.search(line)
                if m:
                    sent = int(m.group(1))
                    total_sent += sent
                    w.writerow([datetime.now().isoformat(), "SEND", sent,
                                m.group(3), m.group(4)])

                m = cons_pat.search(line)
                if m:
                    recv = int(m.group(1))
                    total_recv += recv
                    w.writerow([datetime.now().isoformat(), "RECV", recv, "", ""])

                m = stat_pat.search(line)
                if m:
                    w.writerow([datetime.now().isoformat(), "STAT", "",
                                m.group(1), m.group(2)])
    except KeyboardInterrupt:
        print("\n[*] Interrupted")
    finally:
        ser.close()

    print("\n" + "=" * 60)
    print("STREAM BUFFER SUMMARY")
    print("=" * 60)
    print(f"  Total bytes sent     : {total_sent}")
    print(f"  Total bytes received : {total_recv}")
    print(f"  Difference           : {total_sent - total_recv} (in buffer)")
    print(f"  CSV: {csv_file}")
    print("=" * 60)


if __name__ == "__main__":
    main()

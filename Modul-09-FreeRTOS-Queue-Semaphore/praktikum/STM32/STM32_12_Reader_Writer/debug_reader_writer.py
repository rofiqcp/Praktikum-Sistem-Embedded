#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_12_Reader_Writer
Monitors reader-writer lock patterns, concurrency, and fairness.
"""

import serial
import sys
import csv
import time
import signal
import argparse
import re
from datetime import datetime
from collections import defaultdict

running = True

def signal_handler(sig, frame):
    global running
    print("\n[INFO] Ctrl+C detected, stopping capture...")
    running = False

signal.signal(signal.SIGINT, signal_handler)

def parse_args():
    parser = argparse.ArgumentParser(description="Reader-Writer Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='reader_writer_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'read_ops': defaultdict(int),
        'write_ops': defaultdict(int),
        'concurrent_readers': [],
        'max_concurrent': 0,
        'writer_waits': [],
        'data_versions': [],
    }

    try:
        ser = serial.Serial(args.port, args.baudrate, timeout=1)
        print(f"[CONNECTED] {args.port}")
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {args.port}: {e}")
        sys.exit(1)

    start_time = time.time()

    with open(args.output, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'elapsed_s', 'role', 'id', 'event', 'concurrent', 'version', 'wait_ms', 'raw'])

        print(f"\n[CAPTURING]...")
        print("-" * 60)

        while running and (time.time() - start_time) < args.duration:
            try:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    if not line:
                        continue
                    elapsed = time.time() - start_time
                    ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                    stats['total_lines'] += 1
                    print(f"[{ts}] {line}")

                    m = re.search(r'\[Reader(\d+)\] Read:.*ver=(\d+).*concurrent_readers=(\d+)', line)
                    if m:
                        rid = int(m.group(1))
                        ver = int(m.group(2))
                        conc = int(m.group(3))
                        stats['read_ops'][rid] += 1
                        stats['concurrent_readers'].append(conc)
                        stats['data_versions'].append(ver)
                        if conc > stats['max_concurrent']:
                            stats['max_concurrent'] = conc
                        writer.writerow([ts, f"{elapsed:.3f}", 'Reader', rid, 'READ', conc, ver, '', line])

                    m = re.search(r'\[Writer(\d+)\] WROTE:.*ver=(\d+).*waited=(\d+)ms', line)
                    if m:
                        wid = int(m.group(1))
                        ver = int(m.group(2))
                        wait = int(m.group(3))
                        stats['write_ops'][wid] += 1
                        stats['writer_waits'].append(wait)
                        stats['data_versions'].append(ver)
                        writer.writerow([ts, f"{elapsed:.3f}", 'Writer', wid, 'WRITE', '', ver, wait, line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    total_reads = sum(stats['read_ops'].values())
    total_writes = sum(stats['write_ops'].values())

    print("\n" + "=" * 60)
    print("       READER-WRITER - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:           {duration:.1f}s")
    print(f"  Total reads:        {total_reads}")
    print(f"  Total writes:       {total_writes}")

    if total_writes > 0:
        print(f"  Read/Write ratio:   {total_reads/total_writes:.1f}:1")

    print(f"  Max concurrent readers: {stats['max_concurrent']}")

    if stats['concurrent_readers']:
        avg = sum(stats['concurrent_readers']) / len(stats['concurrent_readers'])
        print(f"  Avg concurrent readers: {avg:.1f}")

    print(f"\n  Per-reader ops:")
    for rid in sorted(stats['read_ops'].keys()):
        print(f"    Reader{rid}: {stats['read_ops'][rid]}")

    print(f"\n  Per-writer ops:")
    for wid in sorted(stats['write_ops'].keys()):
        print(f"    Writer{wid}: {stats['write_ops'][wid]}")

    if stats['writer_waits']:
        ww = stats['writer_waits']
        print(f"\n  Writer wait times:")
        print(f"    Min: {min(ww)}ms, Max: {max(ww)}ms, Avg: {sum(ww)/len(ww):.0f}ms")

    if stats['data_versions']:
        print(f"\n  Final data version: {max(stats['data_versions'])}")

    # Fairness analysis
    if stats['read_ops']:
        read_vals = list(stats['read_ops'].values())
        if len(read_vals) > 1:
            avg_reads = sum(read_vals) / len(read_vals)
            max_dev = max(abs(v - avg_reads) for v in read_vals)
            fairness = 1.0 - (max_dev / avg_reads) if avg_reads > 0 else 0
            print(f"\n  Reader fairness index: {fairness:.2f} (1.0 = perfect)")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

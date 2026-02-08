#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_11_Producer_Consumer
Monitors producer-consumer rates, buffer fill levels, and throughput.
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
    parser = argparse.ArgumentParser(description="Producer-Consumer Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='producer_consumer_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'produced': defaultdict(int),
        'dropped': defaultdict(int),
        'consumed': 0,
        'fill_levels': [],
        'latencies': [],
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
        writer.writerow(['timestamp', 'elapsed_s', 'source', 'event', 'producer', 'seq', 'fill', 'latency_ms', 'raw'])

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

                    m = re.search(r'\[Producer(\d+)\] Sent #(\d+).*fill=(\d+)/\d+', line)
                    if m:
                        pid = int(m.group(1))
                        seq = int(m.group(2))
                        fill = int(m.group(3))
                        stats['produced'][pid] += 1
                        stats['fill_levels'].append(fill)
                        writer.writerow([ts, f"{elapsed:.3f}", 'Producer', 'SEND', pid, seq, fill, '', line])

                    m = re.search(r'\[Producer(\d+)\] BUFFER FULL.*Dropped #(\d+)', line)
                    if m:
                        pid = int(m.group(1))
                        stats['dropped'][pid] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Producer', 'DROP', pid, m.group(2), '', '', line])

                    m = re.search(r'\[Consumer\] #(\d+) from P(\d+):.*latency=(\d+)ms.*fill=(\d+)', line)
                    if m:
                        stats['consumed'] = int(m.group(1))
                        latency = int(m.group(3))
                        fill = int(m.group(4))
                        stats['latencies'].append(latency)
                        stats['fill_levels'].append(fill)
                        writer.writerow([ts, f"{elapsed:.3f}", 'Consumer', 'CONSUME', m.group(2), m.group(1), fill, latency, line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    total_prod = sum(stats['produced'].values())
    total_drop = sum(stats['dropped'].values())

    print("\n" + "=" * 60)
    print("    PRODUCER-CONSUMER - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:          {duration:.1f}s")
    print(f"  Total produced:    {total_prod}")
    print(f"  Total consumed:    {stats['consumed']}")
    print(f"  Total dropped:     {total_drop}")

    if total_prod > 0:
        print(f"  Drop rate:         {total_drop/total_prod*100:.1f}%")
        print(f"  Throughput (prod): {total_prod/duration:.1f} items/s")

    if stats['consumed'] > 0:
        print(f"  Throughput (cons): {stats['consumed']/duration:.1f} items/s")

    print(f"\n  Per-producer:")
    for pid in sorted(stats['produced'].keys()):
        print(f"    P{pid}: produced={stats['produced'][pid]}, dropped={stats['dropped'].get(pid,0)}")

    if stats['fill_levels']:
        fl = stats['fill_levels']
        print(f"\n  Buffer fill levels:")
        print(f"    Min: {min(fl)}, Max: {max(fl)}, Avg: {sum(fl)/len(fl):.1f}")

    if stats['latencies']:
        lat = stats['latencies']
        print(f"\n  Consumer latency:")
        print(f"    Min: {min(lat)}ms, Max: {max(lat)}ms, Avg: {sum(lat)/len(lat):.0f}ms")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

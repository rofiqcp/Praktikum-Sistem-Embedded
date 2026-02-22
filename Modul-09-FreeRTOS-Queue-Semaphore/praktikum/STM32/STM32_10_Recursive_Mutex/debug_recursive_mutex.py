#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_10_Recursive_Mutex
Monitors recursive mutex nesting depth and resource access patterns.
"""

import serial
import sys
import csv
import time
import signal
import argparse
import re
from datetime import datetime

running = True

def signal_handler(sig, frame):
    global running
    print("\n[INFO] Ctrl+C detected, stopping capture...")
    running = False

signal.signal(signal.SIGINT, signal_handler)

def parse_args():
    parser = argparse.ArgumentParser(description="Recursive Mutex Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='recursive_mutex_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'operations': 0,
        'max_depth': 0,
        'depths_seen': [],
        'resource_values': [],
        'task_a_ops': 0,
        'task_b_ops': 0,
        'timeouts': 0,
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
        writer.writerow(['timestamp', 'elapsed_s', 'task', 'event', 'depth', 'resource', 'raw'])

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

                    m = re.search(r'\[Task(\w)\] === Operation #(\d+)', line)
                    if m:
                        stats['operations'] = int(m.group(2))
                        stats['task_a_ops'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", f"Task{m.group(1)}", 'OP_START', 0, '', line])

                    m = re.search(r'Depth (\d+): Resource = (\d+)', line)
                    if m:
                        depth = int(m.group(1))
                        res = int(m.group(2))
                        stats['depths_seen'].append(depth)
                        stats['resource_values'].append(res)
                        if depth > stats['max_depth']:
                            stats['max_depth'] = depth
                        writer.writerow([ts, f"{elapsed:.3f}", '', 'RESOURCE', depth, res, line])

                    if '[TaskB] GOT mutex' in line:
                        stats['task_b_ops'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'TaskB', 'GOT_MUTEX', '', '', line])

                    if 'TIMEOUT' in line:
                        stats['timeouts'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", '', 'TIMEOUT', '', '', line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("     RECURSIVE MUTEX - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:          {duration:.1f}s")
    print(f"  Total operations:  {stats['operations']}")
    print(f"  TaskA operations:  {stats['task_a_ops']}")
    print(f"  TaskB operations:  {stats['task_b_ops']}")
    print(f"  Max nesting depth: {stats['max_depth']}")
    print(f"  Timeouts:          {stats['timeouts']}")

    if stats['depths_seen']:
        from collections import Counter
        depth_dist = Counter(stats['depths_seen'])
        print(f"\n  Depth distribution:")
        for d in sorted(depth_dist.keys()):
            print(f"    Depth {d}: {depth_dist[d]} accesses")

    if stats['resource_values']:
        print(f"\n  Resource values:")
        print(f"    Final: {stats['resource_values'][-1]}")
        print(f"    Max:   {max(stats['resource_values'])}")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

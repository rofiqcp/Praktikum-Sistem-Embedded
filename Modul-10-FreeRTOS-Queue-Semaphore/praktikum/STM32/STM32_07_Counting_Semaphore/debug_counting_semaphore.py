#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_07_Counting_Semaphore
Monitors resource pool usage, tracks concurrency and fairness.
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
    parser = argparse.ArgumentParser(description="Counting Semaphore Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='counting_semaphore_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'acquires': defaultdict(int),
        'releases': defaultdict(int),
        'timeouts': defaultdict(int),
        'wait_times': defaultdict(list),
        'concurrent_levels': [],
        'max_concurrent': 0,
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
        writer.writerow(['timestamp', 'elapsed_s', 'worker', 'event', 'users', 'wait_ms', 'raw'])

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

                    m = re.search(r'\[Worker(\d+)\] ACQUIRED.*users: (\d+)/\d+, waited: (\d+)ms', line)
                    if m:
                        wid = int(m.group(1))
                        users = int(m.group(2))
                        wait = int(m.group(3))
                        stats['acquires'][wid] += 1
                        stats['wait_times'][wid].append(wait)
                        stats['concurrent_levels'].append(users)
                        if users > stats['max_concurrent']:
                            stats['max_concurrent'] = users
                        writer.writerow([ts, f"{elapsed:.3f}", f"Worker{wid}", 'ACQUIRE', users, wait, line])

                    m = re.search(r'\[Worker(\d+)\] RELEASED.*users: (\d+)', line)
                    if m:
                        wid = int(m.group(1))
                        users = int(m.group(2))
                        stats['releases'][wid] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", f"Worker{wid}", 'RELEASE', users, '', line])

                    m = re.search(r'\[Worker(\d+)\] TIMEOUT', line)
                    if m:
                        wid = int(m.group(1))
                        stats['timeouts'][wid] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", f"Worker{wid}", 'TIMEOUT', '', '', line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("    COUNTING SEMAPHORE - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:        {duration:.1f}s")
    print(f"  Max concurrent:  {stats['max_concurrent']}")

    if stats['concurrent_levels']:
        avg = sum(stats['concurrent_levels']) / len(stats['concurrent_levels'])
        print(f"  Avg concurrent:  {avg:.1f}")

    print(f"\n  Per-worker statistics:")
    for wid in sorted(set(list(stats['acquires'].keys()) + list(stats['timeouts'].keys()))):
        acq = stats['acquires'].get(wid, 0)
        rel = stats['releases'].get(wid, 0)
        tout = stats['timeouts'].get(wid, 0)
        waits = stats['wait_times'].get(wid, [])
        avg_wait = sum(waits) / len(waits) if waits else 0
        print(f"    Worker{wid}: acq={acq}, rel={rel}, timeout={tout}, avg_wait={avg_wait:.0f}ms")

    total_acq = sum(stats['acquires'].values())
    total_tout = sum(stats['timeouts'].values())
    if total_acq + total_tout > 0:
        print(f"\n  Overall success rate: {total_acq/(total_acq+total_tout)*100:.1f}%")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

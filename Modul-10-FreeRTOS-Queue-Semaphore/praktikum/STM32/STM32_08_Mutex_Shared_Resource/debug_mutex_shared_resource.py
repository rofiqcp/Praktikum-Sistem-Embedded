#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_08_Mutex_Shared_Resource
Monitors race condition vs mutex-protected counter increments.
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
    parser = argparse.ArgumentParser(description="Mutex Shared Resource Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='mutex_shared_resource_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'unsafe_result': None,
        'safe_result': None,
        'expected': None,
        'unsafe_progress': [],
        'safe_progress': [],
        'phase': 'unknown',
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
        writer.writerow(['timestamp', 'elapsed_s', 'phase', 'task', 'progress', 'counter', 'raw'])

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

                    m = re.search(r'\[Unsafe(\d+)\] Progress: (\d+)/(\d+) \(counter=(\d+)\)', line)
                    if m:
                        stats['phase'] = 'unsafe'
                        stats['unsafe_progress'].append(int(m.group(4)))
                        writer.writerow([ts, f"{elapsed:.3f}", 'UNSAFE', f"Task{m.group(1)}",
                                        m.group(2), m.group(4), line])

                    m = re.search(r'\[Safe(\d+)\] Progress: (\d+)/(\d+) \(counter=(\d+)\)', line)
                    if m:
                        stats['phase'] = 'safe'
                        stats['safe_progress'].append(int(m.group(4)))
                        writer.writerow([ts, f"{elapsed:.3f}", 'SAFE', f"Task{m.group(1)}",
                                        m.group(2), m.group(4), line])

                    m = re.search(r'UNSAFE RESULT: (\d+) \(expected: (\d+)\)', line)
                    if m:
                        stats['unsafe_result'] = int(m.group(1))
                        stats['expected'] = int(m.group(2))
                        writer.writerow([ts, f"{elapsed:.3f}", 'RESULT', 'UNSAFE', '', m.group(1), line])

                    m = re.search(r'SAFE RESULT: (\d+)', line)
                    if m:
                        stats['safe_result'] = int(m.group(1))
                        writer.writerow([ts, f"{elapsed:.3f}", 'RESULT', 'SAFE', '', m.group(1), line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("    MUTEX SHARED RESOURCE - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:        {duration:.1f}s")
    print(f"  Total lines:     {stats['total_lines']}")

    if stats['expected']:
        print(f"\n  Expected result: {stats['expected']}")

    if stats['unsafe_result'] is not None:
        lost = stats['expected'] - stats['unsafe_result'] if stats['expected'] else 0
        pct = (lost / stats['expected'] * 100) if stats['expected'] else 0
        print(f"  Unsafe result:   {stats['unsafe_result']} (lost: {lost}, {pct:.1f}%)")
        print(f"  Race condition:  {'YES' if lost > 0 else 'NO'}")

    if stats['safe_result'] is not None:
        lost = stats['expected'] - stats['safe_result'] if stats['expected'] else 0
        print(f"  Safe result:     {stats['safe_result']} (lost: {lost})")
        print(f"  Mutex correct:   {'YES' if lost == 0 else 'NO'}")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

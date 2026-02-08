#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_09_Mutex_Priority_Inversion
Monitors priority inheritance behavior and task scheduling patterns.
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
    parser = argparse.ArgumentParser(description="Priority Inversion Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='priority_inversion_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'high_runs': 0,
        'med_runs': 0,
        'low_runs': 0,
        'inheritance_events': 0,
        'high_wait_times': [],
        'inherited_priorities': [],
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
        writer.writerow(['timestamp', 'elapsed_s', 'task', 'event', 'priority', 'wait_ms', 'raw'])

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

                    m = re.search(r'\[HIGH\s*\] GOT mutex after (\d+)ms.*run#(\d+)', line)
                    if m:
                        wait = int(m.group(1))
                        stats['high_runs'] = int(m.group(2))
                        stats['high_wait_times'].append(wait)
                        writer.writerow([ts, f"{elapsed:.3f}", 'HIGH', 'GOT_MUTEX', '', wait, line])

                    m = re.search(r'\[MEDIUM\].*run#(\d+)', line)
                    if m:
                        stats['med_runs'] = int(m.group(1))
                        writer.writerow([ts, f"{elapsed:.3f}", 'MEDIUM', 'RUN', '', '', line])

                    m = re.search(r'\[LOW\s*\] GOT mutex.*run#(\d+)', line)
                    if m:
                        stats['low_runs'] = int(m.group(1))
                        writer.writerow([ts, f"{elapsed:.3f}", 'LOW', 'GOT_MUTEX', '', '', line])

                    m = re.search(r'Priority INHERITED:.*-> (\d+)', line)
                    if m:
                        stats['inheritance_events'] += 1
                        inherited_pri = int(m.group(1))
                        stats['inherited_priorities'].append(inherited_pri)
                        writer.writerow([ts, f"{elapsed:.3f}", 'LOW', 'INHERITANCE', inherited_pri, '', line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("     PRIORITY INVERSION - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:              {duration:.1f}s")
    print(f"  High task runs:        {stats['high_runs']}")
    print(f"  Medium task runs:      {stats['med_runs']}")
    print(f"  Low task runs:         {stats['low_runs']}")
    print(f"  Inheritance events:    {stats['inheritance_events']}")

    if stats['high_wait_times']:
        wt = stats['high_wait_times']
        print(f"\n  High task mutex wait times:")
        print(f"    Min: {min(wt)}ms")
        print(f"    Max: {max(wt)}ms")
        print(f"    Avg: {sum(wt)/len(wt):.0f}ms")

    if stats['inherited_priorities']:
        print(f"\n  Inherited priority levels: {set(stats['inherited_priorities'])}")
        print(f"  Priority inheritance is WORKING" if stats['inheritance_events'] > 0
              else "  No priority inheritance observed")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

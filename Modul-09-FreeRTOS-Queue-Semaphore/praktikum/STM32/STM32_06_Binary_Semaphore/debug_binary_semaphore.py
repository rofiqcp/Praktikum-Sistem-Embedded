#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_06_Binary_Semaphore
Monitors binary semaphore ISR-to-task signaling, tracks signal loss.
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
    parser = argparse.ArgumentParser(description="Binary Semaphore Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='binary_semaphore_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'isr_count': 0,
        'task_wakeups': 0,
        'sim_presses': 0,
        'wakeup_times': [],
        'last_wakeup': None,
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
        writer.writerow(['timestamp', 'elapsed_s', 'event', 'isr_count', 'task_count', 'raw'])

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

                    m = re.search(r'Semaphore taken #(\d+).*ISR count: (\d+)', line)
                    if m:
                        stats['task_wakeups'] = int(m.group(1))
                        stats['isr_count'] = int(m.group(2))
                        now = time.time()
                        if stats['last_wakeup']:
                            stats['wakeup_times'].append(now - stats['last_wakeup'])
                        stats['last_wakeup'] = now
                        writer.writerow([ts, f"{elapsed:.3f}", 'SEMAPHORE_TAKE', m.group(2), m.group(1), line])

                    if 'Simulated press' in line:
                        stats['sim_presses'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'SIM_PRESS', '', '', line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("     BINARY SEMAPHORE - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:          {duration:.1f}s")
    print(f"  ISR triggers:      {stats['isr_count']}")
    print(f"  Task wakeups:      {stats['task_wakeups']}")
    print(f"  Simulated presses: {stats['sim_presses']}")
    missed = stats['isr_count'] - stats['task_wakeups']
    print(f"  Missed signals:    {missed}")

    if stats['wakeup_times']:
        wt = stats['wakeup_times']
        print(f"\n  Wakeup intervals:")
        print(f"    Min: {min(wt)*1000:.1f}ms")
        print(f"    Max: {max(wt)*1000:.1f}ms")
        print(f"    Avg: {sum(wt)/len(wt)*1000:.1f}ms")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

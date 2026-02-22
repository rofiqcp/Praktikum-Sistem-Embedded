#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_04_Queue_ISR
Monitors ISR-driven queue events, logs button press timing and response latency.
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
    parser = argparse.ArgumentParser(description="Queue ISR Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='queue_isr_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'button_presses': 0,
        'events_processed': 0,
        'press_ticks': [],
        'inter_press_times': [],
        'last_press_time': None,
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
        writer.writerow(['timestamp', 'elapsed_s', 'event', 'press_count', 'tick', 'raw'])

        print(f"\n[CAPTURING] Press PA0 button or wait for simulated presses...")
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

                    m = re.search(r'\[LEDHandler\] Button #(\d+) at tick=(\d+)', line)
                    if m:
                        press_num = int(m.group(1))
                        tick = int(m.group(2))
                        stats['events_processed'] += 1
                        stats['press_ticks'].append(tick)

                        now = time.time()
                        if stats['last_press_time'] is not None:
                            interval = now - stats['last_press_time']
                            stats['inter_press_times'].append(interval)
                        stats['last_press_time'] = now

                        writer.writerow([ts, f"{elapsed:.3f}", 'BUTTON_EVENT', press_num, tick, line])

                    m = re.search(r'Button presses \(ISR\): (\d+)', line)
                    if m:
                        stats['button_presses'] = int(m.group(1))
                        writer.writerow([ts, f"{elapsed:.3f}", 'ISR_COUNT', m.group(1), '', line])

                    if 'Simulated button' in line:
                        writer.writerow([ts, f"{elapsed:.3f}", 'SIM_PRESS', '', '', line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("        QUEUE ISR - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:           {duration:.1f}s")
    print(f"  Total lines:        {stats['total_lines']}")
    print(f"  ISR button presses: {stats['button_presses']}")
    print(f"  Events processed:   {stats['events_processed']}")

    if stats['inter_press_times']:
        intervals = stats['inter_press_times']
        print(f"\n  Inter-press timing:")
        print(f"    Min interval: {min(intervals)*1000:.1f}ms")
        print(f"    Max interval: {max(intervals)*1000:.1f}ms")
        print(f"    Avg interval: {sum(intervals)/len(intervals)*1000:.1f}ms")

    if stats['press_ticks'] and len(stats['press_ticks']) > 1:
        tick_diffs = [stats['press_ticks'][i+1] - stats['press_ticks'][i]
                      for i in range(len(stats['press_ticks'])-1)]
        print(f"\n  Tick intervals:")
        print(f"    Min: {min(tick_diffs)} ticks")
        print(f"    Max: {max(tick_diffs)} ticks")
        print(f"    Avg: {sum(tick_diffs)/len(tick_diffs):.0f} ticks")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

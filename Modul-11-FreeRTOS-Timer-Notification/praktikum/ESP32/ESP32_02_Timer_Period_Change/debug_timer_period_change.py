#!/usr/bin/env python3
"""
Debug Script: ESP32_02_Timer_Period_Change
============================================
Monitors timer period changes via button press, tracks blink speeds,
logs transitions, and measures actual toggle intervals.

Usage:
    python debug_timer_period_change.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
"""

import serial
import serial.tools.list_ports
import re
import sys
import argparse
import time
import csv
from datetime import datetime
from collections import defaultdict

def auto_detect_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if any(x in (p.vid or 0, p.pid or 0) for x in [0x10C4, 0x1A86, 0x0403]):
            return p.device
        if any(x in p.description.lower() for x in ['cp210', 'ch340', 'ftdi', 'usb']):
            return p.device
    return ports[0].device if ports else '/dev/ttyUSB0'

def main():
    parser = argparse.ArgumentParser(description='Timer Period Change Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"timer_period_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Timer Period Change Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"{'='*60}")

    stats = {
        'blinks': 0, 'button_presses': 0, 'period_changes': 0,
        'errors': 0, 'lines': 0,
    }
    speed_history = []
    blink_timestamps = []
    period_intervals = defaultdict(list)
    current_speed = 'NORMAL(1000ms)'

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Press BOOT button to change speed. (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'speed', 'period_ms',
                           'led', 'toggle_count', 'button_presses', 'tick', 'raw'])

            while True:
                if args.duration > 0 and (time.time() - start_time) > args.duration:
                    break

                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace').strip()
                        if not line:
                            continue
                        stats['lines'] += 1
                        elapsed = time.time() - start_time
                        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                        event = ''
                        speed = ''
                        period = ''
                        led = ''
                        toggles = ''
                        presses = ''
                        tick = ''

                        if 'EVENT,BLINK' in line:
                            event = 'BLINK'
                            stats['blinks'] += 1
                            blink_timestamps.append(elapsed)
                            m = re.search(r'speed=(\S+)', line)
                            if m: speed = m.group(1); current_speed = speed
                            m = re.search(r'period=(\d+)', line)
                            if m: period = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m: led = m.group(1)
                            m = re.search(r'toggle_count=(\d+)', line)
                            if m: toggles = m.group(1)
                            if period and len(blink_timestamps) >= 2:
                                interval = blink_timestamps[-1] - blink_timestamps[-2]
                                period_intervals[period].append(interval * 1000)

                        elif 'BUTTON_PRESS' in line:
                            event = 'BUTTON'
                            stats['button_presses'] += 1
                            m = re.search(r'new_speed=(\S+)', line)
                            if m: speed = m.group(1)
                            speed_history.append((elapsed, speed))

                        elif 'PERIOD_CHANGED' in line:
                            event = 'PERIOD_CHANGE'
                            stats['period_changes'] += 1
                            m = re.search(r'new_period=(\d+)', line)
                            if m: period = m.group(1)
                            blink_timestamps.clear()

                        elif 'STATUS' in line:
                            event = 'STATUS'

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)
                        m2 = re.search(r'presses=(\d+)', line)
                        if m2: presses = m2.group(1)

                        color = '\033[0m'
                        if event == 'BUTTON': color = '\033[93m'
                        elif event == 'PERIOD_CHANGE': color = '\033[96m'
                        elif event == 'BLINK' and led == 'ON': color = '\033[92m'
                        elif event == 'BLINK' and led == 'OFF': color = '\033[94m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, speed, period,
                                       led, toggles, presses, tick, line])

                    except UnicodeDecodeError:
                        pass

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n[INFO] Interrupted")
    finally:
        if 'ser' in locals(): ser.close()

    print(f"\n{'='*60}")
    print(f"  SUMMARY - Timer Period Change")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']} | Blinks: {stats['blinks']}")
    print(f"  Button presses: {stats['button_presses']}")
    print(f"  Period changes: {stats['period_changes']}")

    if speed_history:
        print(f"\n  Speed change history:")
        for t, s in speed_history:
            print(f"    [{t:7.2f}s] -> {s}")

    if period_intervals:
        print(f"\n  Actual interval analysis (ms):")
        for period, intervals in sorted(period_intervals.items()):
            if len(intervals) >= 2:
                avg = sum(intervals) / len(intervals)
                print(f"    Period {period}ms: avg={avg:.1f}ms, min={min(intervals):.1f}ms, "
                      f"max={max(intervals):.1f}ms, samples={len(intervals)}")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

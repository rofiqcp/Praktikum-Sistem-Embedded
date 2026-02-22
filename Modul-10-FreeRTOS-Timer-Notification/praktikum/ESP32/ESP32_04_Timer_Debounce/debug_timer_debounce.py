#!/usr/bin/env python3
"""
Debug Script: ESP32_04_Timer_Debounce
=======================================
Monitors button debounce effectiveness. Tracks raw ISR edges vs
debounced presses, calculates filter efficiency, logs bounce patterns.

Usage:
    python debug_timer_debounce.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
"""

import serial
import serial.tools.list_ports
import re
import sys
import argparse
import time
import csv
from datetime import datetime

def auto_detect_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if any(x in (p.vid or 0, p.pid or 0) for x in [0x10C4, 0x1A86, 0x0403]):
            return p.device
        if any(x in p.description.lower() for x in ['cp210', 'ch340', 'ftdi', 'usb']):
            return p.device
    return ports[0].device if ports else '/dev/ttyUSB0'

def main():
    parser = argparse.ArgumentParser(description='Timer Debounce Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"debounce_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Timer Debounce Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'presses': 0, 'releases': 0,
        'raw_edges': 0, 'isr_triggers': 0, 'efficiency_samples': [],
    }
    press_timestamps = []

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Press BOOT button to test debounce. (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'press_count', 'raw_edges',
                           'isr_triggers', 'led', 'filter_efficiency', 'raw'])

            while True:
                if args.duration > 0 and (time.time() - start_time) > args.duration:
                    break

                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace').strip()
                        if not line: continue
                        stats['lines'] += 1
                        elapsed = time.time() - start_time
                        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                        event = ''
                        presses = ''
                        raw = ''
                        isr = ''
                        led = ''
                        eff = ''

                        if 'DEBOUNCED_PRESS' in line:
                            event = 'PRESS'
                            stats['presses'] += 1
                            press_timestamps.append(elapsed)
                            m = re.search(r'count=(\d+)', line)
                            if m: presses = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m: led = m.group(1)
                            m = re.search(r'raw_edges=(\d+)', line)
                            if m: raw = m.group(1); stats['raw_edges'] = int(m.group(1))
                            m = re.search(r'isr_triggers=(\d+)', line)
                            if m: isr = m.group(1); stats['isr_triggers'] = int(m.group(1))

                        elif 'DEBOUNCED_RELEASE' in line:
                            event = 'RELEASE'
                            stats['releases'] += 1

                        elif 'FILTER' in line:
                            event = 'FILTER_STATS'
                            m = re.search(r'filtered_bounces=(\d+)', line)
                            if m: pass

                        elif 'STATUS' in line:
                            event = 'STATUS'
                            m = re.search(r'filter_efficiency=([\d.]+)', line)
                            if m:
                                eff = m.group(1)
                                stats['efficiency_samples'].append(float(eff.replace('%', '')))

                        color = '\033[0m'
                        if event == 'PRESS': color = '\033[92m'
                        elif event == 'RELEASE': color = '\033[94m'
                        elif event == 'FILTER_STATS': color = '\033[93m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, presses, raw,
                                       isr, led, eff, line])

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
    print(f"  SUMMARY - Timer Debounce")
    print(f"{'='*60}")
    print(f"  Lines captured:      {stats['lines']}")
    print(f"  Debounced presses:   {stats['presses']}")
    print(f"  Debounced releases:  {stats['releases']}")
    print(f"  Raw ISR edges:       {stats['raw_edges']}")
    print(f"  ISR triggers:        {stats['isr_triggers']}")

    total_valid = stats['presses'] + stats['releases']
    if stats['raw_edges'] > 0:
        eff = (total_valid / stats['raw_edges']) * 100
        bounces_filtered = stats['raw_edges'] - total_valid
        print(f"  Bounces filtered:    {bounces_filtered}")
        print(f"  Filter efficiency:   {eff:.1f}% (lower = more bouncing filtered)")
        print(f"  Avg edges per press: {stats['raw_edges'] / max(1, stats['presses']):.1f}")

    if len(press_timestamps) >= 2:
        intervals = [(press_timestamps[i+1] - press_timestamps[i])
                     for i in range(len(press_timestamps)-1)]
        avg = sum(intervals) / len(intervals)
        print(f"  Avg time between presses: {avg:.2f}s")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

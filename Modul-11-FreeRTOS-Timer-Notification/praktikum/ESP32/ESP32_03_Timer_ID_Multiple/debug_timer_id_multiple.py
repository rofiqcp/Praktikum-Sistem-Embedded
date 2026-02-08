#!/usr/bin/env python3
"""
Debug Script: ESP32_03_Timer_ID_Multiple
==========================================
Monitors 3 timers with shared callback, tracks fire rates per timer ID,
verifies expected period ratios, and logs LED states.

Usage:
    python debug_timer_id_multiple.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
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
    parser = argparse.ArgumentParser(description='Timer ID Multiple Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"timer_multi_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Timer ID Multiple Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"  Expected: Timer0@500ms, Timer1@1000ms, Timer2@2000ms")
    print(f"{'='*60}")

    stats = {'lines': 0, 'errors': 0}
    timer_fires = defaultdict(int)
    timer_timestamps = defaultdict(list)
    timer_leds = defaultdict(str)

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Listening... (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'timer_id', 'timer_name',
                           'led', 'gpio', 'count', 'period_ms', 'tick', 'raw'])

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
                        timer_id = ''
                        timer_name = ''
                        led = ''
                        gpio = ''
                        count = ''
                        period = ''
                        tick = ''

                        if 'TIMER_FIRE' in line:
                            event = 'FIRE'
                            m = re.search(r'id=(\d+)', line)
                            if m:
                                timer_id = m.group(1)
                                timer_fires[timer_id] += 1
                                timer_timestamps[timer_id].append(elapsed)
                            m = re.search(r'name=(\S+)', line)
                            if m: timer_name = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m: led = m.group(1); timer_leds[timer_id] = led
                            m = re.search(r'gpio=(\d+)', line)
                            if m: gpio = m.group(1)
                            m = re.search(r'count=(\d+)', line)
                            if m: count = m.group(1)
                            m = re.search(r'period=(\d+)', line)
                            if m: period = m.group(1)

                        elif 'RATIO' in line:
                            event = 'RATIO'
                        elif 'STATUS' in line:
                            event = 'STATUS'

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)

                        colors = {
                            '0': '\033[92m', '1': '\033[93m', '2': '\033[96m'
                        }
                        color = colors.get(timer_id, '\033[0m')
                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, timer_id, timer_name,
                                       led, gpio, count, period, tick, line])

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
    print(f"  SUMMARY - Timer ID Multiple")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")

    expected_periods = {'0': 500, '1': 1000, '2': 2000}
    print(f"\n  Timer fire counts:")
    for tid in sorted(timer_fires.keys()):
        print(f"    Timer {tid}: {timer_fires[tid]} fires (expected period: {expected_periods.get(tid, '?')}ms)")

    print(f"\n  Timer interval analysis:")
    for tid in sorted(timer_timestamps.keys()):
        ts_list = timer_timestamps[tid]
        if len(ts_list) >= 2:
            intervals = [(ts_list[i+1] - ts_list[i]) * 1000 for i in range(len(ts_list)-1)]
            avg = sum(intervals) / len(intervals)
            expected = expected_periods.get(tid, 0)
            error_pct = abs(avg - expected) / expected * 100 if expected > 0 else 0
            print(f"    Timer {tid}: avg={avg:.1f}ms (expected {expected}ms, error {error_pct:.1f}%), "
                  f"min={min(intervals):.1f}ms, max={max(intervals):.1f}ms")

    if timer_fires.get('0', 0) > 0 and timer_fires.get('2', 0) > 0:
        ratio = timer_fires['0'] / timer_fires['2']
        print(f"\n  Timer0/Timer2 ratio: {ratio:.2f} (expected ~4.0)")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

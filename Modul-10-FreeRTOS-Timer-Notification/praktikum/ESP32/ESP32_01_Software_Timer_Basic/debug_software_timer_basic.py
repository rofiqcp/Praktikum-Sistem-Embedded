#!/usr/bin/env python3
"""
Debug Script: ESP32_01_Software_Timer_Basic
=============================================
Serial monitor with CSV logging for FreeRTOS software timer demo.
Tracks one-shot and auto-reload timer events, callback context, LED states.

Usage:
    python debug_software_timer_basic.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
"""

import serial
import serial.tools.list_ports
import re
import sys
import os
import argparse
import time
import csv
from datetime import datetime
from collections import defaultdict

def auto_detect_port():
    """Auto-detect ESP32 serial port."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if any(x in (p.vid or 0, p.pid or 0) for x in [0x10C4, 0x1A86, 0x0403]):
            return p.device
        if any(x in p.description.lower() for x in ['cp210', 'ch340', 'ftdi', 'usb']):
            return p.device
    if ports:
        return ports[0].device
    return '/dev/ttyUSB0'

def main():
    parser = argparse.ArgumentParser(description='Software Timer Basic Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port (auto-detect if not specified)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Capture duration in seconds (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"timer_basic_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Software Timer Basic Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port}")
    print(f"  Baud: {args.baud}")
    print(f"  Output: {output_file}")
    print(f"  Duration: {'unlimited' if args.duration == 0 else f'{args.duration}s'}")
    print(f"{'='*60}")

    # Stats
    stats = {
        'oneshot_fires': 0,
        'autoreload_fires': 0,
        'led_on_count': 0,
        'led_off_count': 0,
        'errors': 0,
        'lines': 0,
        'timer_stops': 0,
        'timer_restarts': 0,
    }
    events = []
    autoreload_timestamps = []

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Listening... (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'level', 'tag', 'event_type',
                           'timer_type', 'led_state', 'count', 'tick', 'raw'])

            while True:
                if args.duration > 0 and (time.time() - start_time) > args.duration:
                    print(f"\n[INFO] Duration ({args.duration}s) reached.")
                    break

                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace').strip()
                        if not line:
                            continue

                        stats['lines'] += 1
                        elapsed = time.time() - start_time
                        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                        # Parse log level
                        level = 'INFO'
                        if line.startswith('W ') or 'WARN' in line:
                            level = 'WARN'
                        elif line.startswith('E ') or 'ERROR' in line:
                            level = 'ERROR'
                            stats['errors'] += 1

                        # Detect events
                        event_type = ''
                        timer_type = ''
                        led_state = ''
                        count = ''
                        tick = ''

                        if 'ONE_SHOT' in line and 'FIRED' in line:
                            event_type = 'ONESHOT_FIRE'
                            timer_type = 'one_shot'
                            stats['oneshot_fires'] += 1
                            m = re.search(r'count=(\d+)', line)
                            if m: count = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m: led_state = m.group(1)

                        elif 'AUTO_RELOAD' in line and 'TOGGLE' in line:
                            event_type = 'AUTORELOAD_TOGGLE'
                            timer_type = 'auto_reload'
                            stats['autoreload_fires'] += 1
                            autoreload_timestamps.append(elapsed)
                            m = re.search(r'count=(\d+)', line)
                            if m: count = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m:
                                led_state = m.group(1)
                                if led_state == 'ON':
                                    stats['led_on_count'] += 1
                                else:
                                    stats['led_off_count'] += 1

                        elif 'Stopping' in line:
                            event_type = 'TIMER_STOP'
                            stats['timer_stops'] += 1

                        elif 'Restarting' in line:
                            event_type = 'TIMER_RESTART'
                            stats['timer_restarts'] += 1

                        elif 'STATUS' in line:
                            event_type = 'STATUS'

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)

                        # Color output
                        color = '\033[0m'
                        if event_type == 'ONESHOT_FIRE':
                            color = '\033[93m'  # Yellow
                        elif event_type == 'AUTORELOAD_TOGGLE':
                            color = '\033[92m' if led_state == 'ON' else '\033[94m'
                        elif level == 'WARN':
                            color = '\033[93m'
                        elif level == 'ERROR':
                            color = '\033[91m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', level, 'TIMER_BASIC',
                                       event_type, timer_type, led_state, count, tick, line])

                    except UnicodeDecodeError:
                        pass

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n[INFO] Interrupted by user")
    finally:
        if 'ser' in locals():
            ser.close()

    # Summary
    print(f"\n{'='*60}")
    print(f"  SUMMARY - Software Timer Basic")
    print(f"{'='*60}")
    print(f"  Total lines captured: {stats['lines']}")
    print(f"  One-shot fires:      {stats['oneshot_fires']}")
    print(f"  Auto-reload fires:   {stats['autoreload_fires']}")
    print(f"  LED ON events:       {stats['led_on_count']}")
    print(f"  LED OFF events:      {stats['led_off_count']}")
    print(f"  Timer stops:         {stats['timer_stops']}")
    print(f"  Timer restarts:      {stats['timer_restarts']}")
    print(f"  Errors:              {stats['errors']}")

    if len(autoreload_timestamps) >= 2:
        intervals = [autoreload_timestamps[i+1] - autoreload_timestamps[i]
                     for i in range(len(autoreload_timestamps)-1)]
        avg_interval = sum(intervals) / len(intervals)
        print(f"\n  Auto-reload interval analysis:")
        print(f"    Avg interval:  {avg_interval*1000:.1f} ms")
        print(f"    Min interval:  {min(intervals)*1000:.1f} ms")
        print(f"    Max interval:  {max(intervals)*1000:.1f} ms")

    print(f"\n  CSV saved to: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

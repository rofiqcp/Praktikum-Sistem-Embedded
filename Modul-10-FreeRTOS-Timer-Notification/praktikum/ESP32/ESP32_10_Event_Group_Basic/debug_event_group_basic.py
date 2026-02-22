#!/usr/bin/env python3
"""
Debug Script: ESP32_10_Event_Group_Basic
==========================================
Monitors event group AND/OR logic demo. Tracks sensor ready events,
alerts, wait times, and bit patterns.

Usage:
    python debug_event_group_basic.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
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
    parser = argparse.ArgumentParser(description='Event Group Basic Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"evtgrp_basic_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Event Group Basic Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"  Bit mapping: 0x01=Temp, 0x02=Humid, 0x04=Press")
    print(f"  Alerts:      0x08=TempAlert, 0x10=HumidAlert, 0x20=PressAlert")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'sensor_ready': defaultdict(int), 'all_ready': 0,
        'alerts': defaultdict(int), 'timeouts': 0, 'cycles': 0,
    }
    wait_times = []

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Listening... (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'sensor', 'bits',
                           'wait_ms', 'count', 'tick', 'raw'])

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
                        sensor = ''
                        bits = ''
                        wait_ms = ''
                        count = ''
                        tick = ''

                        if 'SENSOR_READY' in line:
                            event = 'SENSOR_READY'
                            m = re.search(r'name=(\w+)', line)
                            if m: sensor = m.group(1); stats['sensor_ready'][sensor] += 1
                            m = re.search(r'bits_after=0x(\w+)', line)
                            if m: bits = m.group(1)

                        elif 'ALL_SENSORS_READY' in line:
                            event = 'ALL_READY'
                            stats['all_ready'] += 1
                            m = re.search(r'wait_time=(\d+)', line)
                            if m:
                                wait_ms = m.group(1)
                                wait_times.append(int(wait_ms))
                            m = re.search(r'count=(\d+)', line)
                            if m: count = m.group(1)

                        elif 'ALERT_RECEIVED' in line:
                            event = 'ALERT_RECEIVED'
                            m = re.search(r'type=(\w+)', line)
                            if m: sensor = m.group(1); stats['alerts'][sensor] += 1
                            m = re.search(r'bits=0x(\w+)', line)
                            if m: bits = m.group(1)

                        elif 'ALERT_SET' in line:
                            event = 'ALERT_SET'
                            m = re.search(r'type=(\w+)', line)
                            if m: sensor = m.group(1)

                        elif 'AND_WAIT_TIMEOUT' in line:
                            event = 'TIMEOUT'
                            stats['timeouts'] += 1

                        elif 'AND_WAIT_START' in line:
                            event = 'AND_WAIT'
                            stats['cycles'] += 1

                        elif 'STATUS' in line:
                            event = 'STATUS'
                            m = re.search(r'current_bits=0x(\w+)', line)
                            if m: bits = m.group(1)

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)

                        color = '\033[0m'
                        if event == 'SENSOR_READY': color = '\033[96m'
                        elif event == 'ALL_READY': color = '\033[92m'
                        elif 'ALERT' in event: color = '\033[93m'
                        elif event == 'TIMEOUT': color = '\033[91m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, sensor, bits,
                                       wait_ms, count, tick, line])

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
    print(f"  SUMMARY - Event Group Basic")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  AND wait cycles:     {stats['cycles']}")
    print(f"  ALL sensors ready:   {stats['all_ready']}")
    print(f"  AND wait timeouts:   {stats['timeouts']}")

    print(f"\n  Sensor ready counts:")
    for s, c in sorted(stats['sensor_ready'].items()):
        print(f"    {s}: {c}")

    print(f"\n  Alert counts:")
    for s, c in sorted(stats['alerts'].items()):
        print(f"    {s}: {c}")

    if wait_times:
        avg = sum(wait_times) / len(wait_times)
        print(f"\n  AND-wait time (ms): avg={avg:.0f}, min={min(wait_times)}, max={max(wait_times)}")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

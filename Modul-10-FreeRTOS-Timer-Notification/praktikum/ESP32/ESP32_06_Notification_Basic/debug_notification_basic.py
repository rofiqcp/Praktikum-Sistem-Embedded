#!/usr/bin/env python3
"""
Debug Script: ESP32_06_Notification_Basic
===========================================
Monitors task notification vs binary semaphore comparison.
Tracks latency per method, press counts, mode switches.

Usage:
    python debug_notification_basic.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
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
    parser = argparse.ArgumentParser(description='Notification Basic Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"notify_basic_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Notification Basic Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"  Comparing: Task Notification vs Binary Semaphore")
    print(f"{'='*60}")

    stats = {'lines': 0, 'notify_presses': 0, 'sema_presses': 0, 'mode_switches': 0}
    latencies = defaultdict(list)
    current_mode = 'NOTIFICATION'

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Press BOOT button. (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'method', 'latency_us',
                           'avg_latency_us', 'led', 'count', 'tick', 'raw'])

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
                        method = ''
                        latency = ''
                        avg_lat = ''
                        led = ''
                        count = ''
                        tick = ''

                        if 'EVENT,NOTIFICATION' in line:
                            event = 'NOTIFY_PRESS'
                            method = 'NOTIFICATION'
                            stats['notify_presses'] += 1
                            m = re.search(r'latency=(\d+)', line)
                            if m:
                                latency = m.group(1)
                                latencies['notify'].append(int(latency))
                            m = re.search(r'avg_latency=(\d+)', line)
                            if m: avg_lat = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m: led = m.group(1)

                        elif 'EVENT,SEMAPHORE' in line:
                            event = 'SEMA_PRESS'
                            method = 'SEMAPHORE'
                            stats['sema_presses'] += 1
                            m = re.search(r'latency=(\d+)', line)
                            if m:
                                latency = m.group(1)
                                latencies['sema'].append(int(latency))
                            m = re.search(r'avg_latency=(\d+)', line)
                            if m: avg_lat = m.group(1)
                            m = re.search(r'led=(\w+)', line)
                            if m: led = m.group(1)

                        elif 'SWITCH' in line:
                            event = 'MODE_SWITCH'
                            stats['mode_switches'] += 1
                            if 'Semaphore' in line: current_mode = 'SEMAPHORE'
                            elif 'Notification' in line: current_mode = 'NOTIFICATION'

                        elif 'COMPARE' in line:
                            event = 'COMPARE'
                        elif 'STATUS' in line:
                            event = 'STATUS'

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)
                        m = re.search(r'count=(\d+)', line)
                        if m: count = m.group(1)

                        color = '\033[0m'
                        if event == 'NOTIFY_PRESS': color = '\033[92m'
                        elif event == 'SEMA_PRESS': color = '\033[96m'
                        elif event == 'MODE_SWITCH': color = '\033[93m'
                        elif event == 'COMPARE': color = '\033[95m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, method, latency,
                                       avg_lat, led, count, tick, line])

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
    print(f"  SUMMARY - Notification vs Semaphore")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Notification presses: {stats['notify_presses']}")
    print(f"  Semaphore presses:    {stats['sema_presses']}")
    print(f"  Mode switches:        {stats['mode_switches']}")

    for method_key, method_name in [('notify', 'Task Notification'), ('sema', 'Binary Semaphore')]:
        lats = latencies[method_key]
        if lats:
            print(f"\n  {method_name} latency (us):")
            print(f"    Samples: {len(lats)}")
            print(f"    Avg:     {sum(lats)/len(lats):.1f}")
            print(f"    Min:     {min(lats)}")
            print(f"    Max:     {max(lats)}")

    if latencies['notify'] and latencies['sema']:
        n_avg = sum(latencies['notify']) / len(latencies['notify'])
        s_avg = sum(latencies['sema']) / len(latencies['sema'])
        diff = s_avg - n_avg
        print(f"\n  Notification is {'faster' if diff > 0 else 'slower'} by {abs(diff):.1f} us")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

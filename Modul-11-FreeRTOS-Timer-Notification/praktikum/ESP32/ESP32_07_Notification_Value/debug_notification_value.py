#!/usr/bin/env python3
"""
Debug Script: ESP32_07_Notification_Value
===========================================
Monitors xTaskNotify value passing, tracks command types,
overwrite vs non-overwrite results, and failures.

Usage:
    python debug_notification_value.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
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
    parser = argparse.ArgumentParser(description='Notification Value Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"notify_value_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Notification Value Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"  Commands: on, off, status, reset, up, down, auto")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'cmds_sent': 0, 'cmds_received': 0,
        'overwrites': 0, 'no_overwrites': 0, 'no_overwrite_fails': 0,
    }
    cmd_counts = defaultdict(int)

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Type commands in serial monitor. (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'cmd_name', 'cmd_value',
                           'mode', 'result', 'total_sent', 'total_received', 'raw'])

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
                        cmd_name = ''
                        cmd_value = ''
                        mode = ''
                        result = ''

                        if 'CMD_RECEIVED' in line:
                            event = 'RECEIVED'
                            stats['cmds_received'] += 1
                            m = re.search(r'name=(\w+)', line)
                            if m: cmd_name = m.group(1); cmd_counts[cmd_name] += 1
                            m = re.search(r'value=0x(\w+)', line)
                            if m: cmd_value = m.group(1)

                        elif 'SEND' in line:
                            event = 'SEND'
                            stats['cmds_sent'] += 1
                            m = re.search(r'value=0x(\w+)', line)
                            if m: cmd_value = m.group(1)
                            m = re.search(r'mode=(\w+)', line)
                            if m: mode = m.group(1)
                            m = re.search(r'result=(\w+)', line)
                            if m: result = m.group(1)
                            if 'WithOverwrite' in line and 'Without' not in line:
                                stats['overwrites'] += 1
                            elif 'WithoutOverwrite' in line:
                                stats['no_overwrites'] += 1
                                if 'FAILED' in line:
                                    stats['no_overwrite_fails'] += 1

                        elif 'AUTO' in line:
                            event = 'AUTO'
                        elif 'ACTION' in line:
                            event = 'ACTION'
                        elif 'STATUS' in line:
                            event = 'STATUS'

                        color = '\033[0m'
                        if event == 'RECEIVED': color = '\033[92m'
                        elif event == 'SEND': color = '\033[96m'
                        elif event == 'AUTO': color = '\033[93m'
                        elif 'FAILED' in line: color = '\033[91m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, cmd_name, cmd_value,
                                       mode, result, stats['cmds_sent'],
                                       stats['cmds_received'], line])

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
    print(f"  SUMMARY - Notification Value")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Commands sent:     {stats['cmds_sent']}")
    print(f"  Commands received: {stats['cmds_received']}")
    print(f"  Overwrites:        {stats['overwrites']}")
    print(f"  No-overwrites:     {stats['no_overwrites']}")
    print(f"  No-overwrite fails:{stats['no_overwrite_fails']}")

    if cmd_counts:
        print(f"\n  Command distribution:")
        for cmd, cnt in sorted(cmd_counts.items(), key=lambda x: -x[1]):
            print(f"    {cmd}: {cnt}")

    if stats['no_overwrites'] > 0:
        fail_rate = stats['no_overwrite_fails'] / stats['no_overwrites'] * 100
        print(f"\n  eSetValueWithoutOverwrite fail rate: {fail_rate:.1f}%")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

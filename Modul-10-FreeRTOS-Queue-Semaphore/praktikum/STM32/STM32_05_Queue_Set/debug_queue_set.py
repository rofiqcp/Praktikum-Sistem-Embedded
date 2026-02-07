#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_05_Queue_Set
Monitors multiplexed queue reception, analyzes source distribution.
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
    parser = argparse.ArgumentParser(description="Queue Set Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='queue_set_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'temp_sent': 0, 'temp_received': 0,
        'alarm_sent': 0, 'alarm_received': 0,
        'temp_values': [],
        'alarm_codes': {},
        'high_temp_warnings': 0,
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
        writer.writerow(['timestamp', 'elapsed_s', 'source', 'event', 'value', 'raw'])

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

                    m = re.search(r'\[TempSender\] Sent temp=([\d.]+)', line)
                    if m:
                        stats['temp_sent'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'TempSender', 'SEND', m.group(1), line])

                    m = re.search(r'\[Receiver\] TEMP #(\d+): ([\d.]+)', line)
                    if m:
                        stats['temp_received'] += 1
                        stats['temp_values'].append(float(m.group(2)))
                        writer.writerow([ts, f"{elapsed:.3f}", 'Receiver', 'TEMP', m.group(2), line])

                    m = re.search(r'\[AlarmSender\] Sent alarm: code=(\d+) msg=(\w+)', line)
                    if m:
                        stats['alarm_sent'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'AlarmSender', 'SEND', m.group(2), line])

                    m = re.search(r'\[Receiver\] ALARM #(\d+): \[(\d+)\] (\w+)', line)
                    if m:
                        stats['alarm_received'] += 1
                        code = m.group(3)
                        stats['alarm_codes'][code] = stats['alarm_codes'].get(code, 0) + 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Receiver', 'ALARM', code, line])

                    if 'HIGH TEMP' in line:
                        stats['high_temp_warnings'] += 1

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("        QUEUE SET - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:           {duration:.1f}s")
    print(f"  Temp sent/recv:     {stats['temp_sent']}/{stats['temp_received']}")
    print(f"  Alarm sent/recv:    {stats['alarm_sent']}/{stats['alarm_received']}")
    print(f"  High temp warnings: {stats['high_temp_warnings']}")

    if stats['temp_values']:
        print(f"\n  Temperature stats:")
        print(f"    Min: {min(stats['temp_values']):.1f}°C")
        print(f"    Max: {max(stats['temp_values']):.1f}°C")
        print(f"    Avg: {sum(stats['temp_values'])/len(stats['temp_values']):.1f}°C")

    if stats['alarm_codes']:
        print(f"\n  Alarm distribution:")
        for code, count in sorted(stats['alarm_codes'].items()):
            print(f"    {code}: {count}")

    total_recv = stats['temp_received'] + stats['alarm_received']
    if total_recv > 0:
        print(f"\n  Source distribution:")
        print(f"    Temp:  {stats['temp_received']}/{total_recv} ({stats['temp_received']/total_recv*100:.0f}%)")
        print(f"    Alarm: {stats['alarm_received']}/{total_recv} ({stats['alarm_received']/total_recv*100:.0f}%)")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

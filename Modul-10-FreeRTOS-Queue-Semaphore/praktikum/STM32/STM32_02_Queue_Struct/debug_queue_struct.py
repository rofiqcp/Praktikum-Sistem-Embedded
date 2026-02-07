#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_02_Queue_Struct
Monitors sensor data via serial, logs to CSV, analyzes per-sensor statistics.
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
    parser = argparse.ArgumentParser(description="Queue Struct Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='queue_struct_log.csv', help='Output CSV file')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'total_processed': 0,
        'sensor_data': {'Temperature': [], 'Humidity': [], 'Light': []},
        'send_count': {'Temperature': 0, 'Humidity': 0, 'Light': 0},
        'queue_full_count': 0,
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
        writer.writerow(['timestamp', 'elapsed_s', 'source', 'sensor_type', 'value', 'tick', 'raw'])

        print(f"\n[CAPTURING] Started at {datetime.now().strftime('%H:%M:%S')}...")
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

                    # Parse sensor sends
                    m = re.search(r'\[Sensor\d\] Sent (\w+): ([\d.]+) \(t=(\d+)\)', line)
                    if m:
                        stype = m.group(1)
                        val = float(m.group(2))
                        tick = int(m.group(3))
                        if stype in stats['send_count']:
                            stats['send_count'][stype] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Sensor', stype, val, tick, line])

                    # Parse processor output
                    m = re.search(r'\[Processor\] #(\d+) .* Type:(\w+) Value:([\d.]+) Time:(\d+)', line)
                    if m:
                        idx = int(m.group(1))
                        stype = m.group(2)
                        val = float(m.group(3))
                        tick = int(m.group(4))
                        stats['total_processed'] = idx
                        if stype in stats['sensor_data']:
                            stats['sensor_data'][stype].append(val)
                        writer.writerow([ts, f"{elapsed:.3f}", 'Processor', stype, val, tick, line])

                    if 'Queue FULL' in line:
                        stats['queue_full_count'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Error', 'QUEUE_FULL', '', '', line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("       QUEUE STRUCT - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:         {duration:.1f}s")
    print(f"  Total lines:      {stats['total_lines']}")
    print(f"  Total processed:  {stats['total_processed']}")
    print(f"  Queue full:       {stats['queue_full_count']}")

    for stype in ['Temperature', 'Humidity', 'Light']:
        data = stats['sensor_data'][stype]
        sent = stats['send_count'][stype]
        print(f"\n  [{stype}]")
        print(f"    Sent: {sent}, Processed: {len(data)}")
        if data:
            print(f"    Min: {min(data):.1f}, Max: {max(data):.1f}, Avg: {sum(data)/len(data):.1f}")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

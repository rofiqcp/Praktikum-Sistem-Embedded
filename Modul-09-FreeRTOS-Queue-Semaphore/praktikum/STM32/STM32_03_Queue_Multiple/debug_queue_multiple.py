#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_03_Queue_Multiple
Monitors command-status pipeline, logs to CSV, analyzes command execution patterns.
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
    parser = argparse.ArgumentParser(description="Queue Multiple Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration (s)')
    parser.add_argument('-o', '--output', default='queue_multiple_log.csv', help='Output CSV')
    return parser.parse_args()

def main():
    args = parse_args()
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}, Duration: {args.duration}s")

    stats = {
        'total_lines': 0,
        'cmds_sent': 0,
        'cmds_executed': 0,
        'statuses': 0,
        'cmd_types': {},
        'exec_times': [],
        'successes': 0,
        'failures': 0,
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
        writer.writerow(['timestamp', 'elapsed_s', 'source', 'event', 'cmd_id', 'cmd_type', 'exec_time_ms', 'raw'])

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

                    m = re.search(r'\[Commander\] Sent CMD #(\d+): (\w+)', line)
                    if m:
                        cid = int(m.group(1))
                        ctype = m.group(2)
                        stats['cmds_sent'] += 1
                        stats['cmd_types'][ctype] = stats['cmd_types'].get(ctype, 0) + 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Commander', 'SEND', cid, ctype, '', line])

                    m = re.search(r'\[Executor\] Executing CMD #(\d+): (\w+)', line)
                    if m:
                        stats['cmds_executed'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Executor', 'EXEC', m.group(1), m.group(2), '', line])

                    m = re.search(r'\[Monitor\] CMD #(\d+): (\w+) \(exec_time=(\d+)ms', line)
                    if m:
                        cid = int(m.group(1))
                        result = m.group(2)
                        etime = int(m.group(3))
                        stats['statuses'] += 1
                        stats['exec_times'].append(etime)
                        if result == 'SUCCESS':
                            stats['successes'] += 1
                        else:
                            stats['failures'] += 1
                        writer.writerow([ts, f"{elapsed:.3f}", 'Monitor', 'STATUS', cid, result, etime, line])

            except Exception as e:
                print(f"[WARN] Read error: {e}")

    ser.close()

    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("       QUEUE MULTIPLE - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:          {duration:.1f}s")
    print(f"  Total lines:       {stats['total_lines']}")
    print(f"  Commands sent:     {stats['cmds_sent']}")
    print(f"  Commands executed: {stats['cmds_executed']}")
    print(f"  Statuses received: {stats['statuses']}")
    print(f"  Successes:         {stats['successes']}")
    print(f"  Failures:          {stats['failures']}")

    if stats['exec_times']:
        print(f"\n  Execution times:")
        print(f"    Min:  {min(stats['exec_times'])}ms")
        print(f"    Max:  {max(stats['exec_times'])}ms")
        print(f"    Avg:  {sum(stats['exec_times'])/len(stats['exec_times']):.1f}ms")

    if stats['cmd_types']:
        print(f"\n  Command distribution:")
        for ctype, count in sorted(stats['cmd_types'].items()):
            print(f"    {ctype}: {count}")

    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()

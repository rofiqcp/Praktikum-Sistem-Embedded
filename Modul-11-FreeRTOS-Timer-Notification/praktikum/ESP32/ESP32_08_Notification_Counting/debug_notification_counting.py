#!/usr/bin/env python3
"""
Debug Script: ESP32_08_Notification_Counting
==============================================
Monitors counting semaphore behavior via task notifications.
Tracks event production, processing, backlog, and burst behavior.

Usage:
    python debug_notification_counting.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
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
    parser = argparse.ArgumentParser(description='Notification Counting Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"notify_counting_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Notification Counting Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'produced': 0, 'processed': 0,
        'bursts_started': 0, 'bursts_completed': 0, 'max_backlog': 0,
    }
    producer_events = defaultdict(int)
    backlog_history = []
    process_timestamps = []

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Listening... (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'producer', 'batch_size',
                           'generated', 'processed', 'backlog', 'tick', 'raw'])

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
                        producer = ''
                        batch = ''
                        gen = ''
                        proc = ''
                        backlog = ''
                        tick = ''

                        if 'EVENT,PRODUCED' in line:
                            event = 'PRODUCED'
                            m = re.search(r'producer=(\d+)', line)
                            if m: producer = m.group(1); producer_events[producer] += 1
                            m = re.search(r'batch=(\d+)', line)
                            if m: batch = m.group(1); stats['produced'] += int(m.group(1))
                            m = re.search(r'total_produced=(\d+)', line)
                            if m: gen = m.group(1)

                        elif 'EVENT,PROCESSED' in line:
                            event = 'PROCESSED'
                            stats['processed'] += 1
                            process_timestamps.append(elapsed)
                            m = re.search(r'generated=(\d+)', line)
                            if m: gen = m.group(1)
                            m = re.search(r'processed=(\d+)', line)
                            if m: proc = m.group(1)
                            m = re.search(r'backlog=(\d+)', line)
                            if m:
                                backlog = m.group(1)
                                bl = int(backlog)
                                backlog_history.append((elapsed, bl))
                                if bl > stats['max_backlog']:
                                    stats['max_backlog'] = bl

                        elif 'BURST_START' in line:
                            event = 'BURST_START'
                            stats['bursts_started'] += 1

                        elif 'BURST_COMPLETE' in line:
                            event = 'BURST_COMPLETE'
                            stats['bursts_completed'] += 1

                        elif 'BURST_GENERATED' in line:
                            event = 'BURST_GEN'

                        elif 'STATUS' in line:
                            event = 'STATUS'
                            m = re.search(r'backlog=(\d+)', line)
                            if m: backlog = m.group(1)

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)

                        color = '\033[0m'
                        if event == 'PRODUCED': color = '\033[96m'
                        elif event == 'PROCESSED': color = '\033[92m'
                        elif 'BURST' in event: color = '\033[93m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, producer, batch,
                                       gen, proc, backlog, tick, line])

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
    print(f"  SUMMARY - Notification Counting")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Events produced:   {stats['produced']}")
    print(f"  Events processed:  {stats['processed']}")
    print(f"  Max backlog:       {stats['max_backlog']}")
    print(f"  Bursts:            {stats['bursts_started']} started, {stats['bursts_completed']} completed")

    if producer_events:
        print(f"\n  Producer distribution:")
        for pid, cnt in sorted(producer_events.items()):
            print(f"    Producer {pid}: {cnt} batches")

    if len(process_timestamps) >= 2:
        intervals = [(process_timestamps[i+1] - process_timestamps[i]) * 1000
                     for i in range(len(process_timestamps)-1)]
        avg = sum(intervals) / len(intervals)
        print(f"\n  Processing interval: avg={avg:.1f}ms, min={min(intervals):.1f}ms, max={max(intervals):.1f}ms")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

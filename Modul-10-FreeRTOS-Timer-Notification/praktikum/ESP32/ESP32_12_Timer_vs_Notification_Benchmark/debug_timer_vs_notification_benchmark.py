#!/usr/bin/env python3
"""
Debug Script: ESP32_12_Timer_vs_Notification_Benchmark
========================================================
Monitors benchmark results comparing SW Timer, Task Notification,
and Binary Semaphore latency. Captures detailed stats and comparison.

Usage:
    python debug_timer_vs_notification_benchmark.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 120]
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
    parser = argparse.ArgumentParser(description='Timer vs Notification Benchmark Debug')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"benchmark_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Timer vs Notification Benchmark Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"  Mechanisms: SW Timer, Task Notification, Binary Semaphore")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'bench_starts': 0, 'bench_done': 0,
        'results': 0, 'winners': defaultdict(int),
    }
    bench_results = defaultdict(lambda: {'count': 0, 'min': 0, 'max': 0, 'avg': 0, 'stddev': 0})
    progress = defaultdict(list)

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Waiting for benchmark... (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'method', 'iterations',
                           'min_us', 'max_us', 'avg_us', 'stddev', 'progress', 'raw'])

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
                        iterations = ''
                        min_us = ''
                        max_us = ''
                        avg_us = ''
                        stddev = ''
                        prog = ''

                        if 'BENCH,START' in line:
                            event = 'BENCH_START'
                            stats['bench_starts'] += 1
                            m = re.search(r'START,(\w+)', line)
                            if m: method = m.group(1)
                            m = re.search(r'(\d+) iterations', line)
                            if m: iterations = m.group(1)

                        elif 'BENCH,PROGRESS' in line:
                            event = 'PROGRESS'
                            m = re.search(r'PROGRESS,(\w+)', line)
                            if m: method = m.group(1)
                            m = re.search(r'(\d+)/(\d+)', line)
                            if m: prog = f"{m.group(1)}/{m.group(2)}"
                            m = re.search(r'avg=(\d+)', line)
                            if m:
                                avg_us = m.group(1)
                                progress[method].append(int(avg_us))

                        elif 'BENCH,DONE' in line:
                            event = 'BENCH_DONE'
                            stats['bench_done'] += 1
                            m = re.search(r'DONE,(\w+)', line)
                            if m: method = m.group(1)
                            m = re.search(r'count=(\d+)', line)
                            if m: iterations = m.group(1)
                            m = re.search(r'min=(\d+)', line)
                            if m: min_us = m.group(1)
                            m = re.search(r'max=(\d+)', line)
                            if m: max_us = m.group(1)
                            m = re.search(r'avg=(\d+)', line)
                            if m: avg_us = m.group(1)
                            m = re.search(r'stddev=([\d.]+)', line)
                            if m: stddev = m.group(1)
                            if method:
                                bench_results[method] = {
                                    'count': iterations, 'min': min_us,
                                    'max': max_us, 'avg': avg_us, 'stddev': stddev
                                }

                        elif 'RESULT,' in line and 'RESULT' in line:
                            event = 'RESULT'
                            stats['results'] += 1
                            parts = line.split(',')
                            if len(parts) >= 6:
                                method = parts[1] if len(parts) > 1 else ''

                        elif 'WINNER' in line:
                            event = 'WINNER'
                            m = re.search(r'WINNER,(\w+)', line)
                            if m:
                                method = m.group(1)
                                stats['winners'][method] += 1

                        elif 'COMPARE,' in line and 'slower' in line:
                            event = 'COMPARE'

                        elif 'BENCHMARK' in line and '=====' in line:
                            event = 'SECTION'

                        color = '\033[0m'
                        if event == 'BENCH_START': color = '\033[96m'
                        elif event == 'BENCH_DONE': color = '\033[92m'
                        elif event == 'WINNER': color = '\033[95m'
                        elif event == 'RESULT': color = '\033[93m'
                        elif event == 'COMPARE': color = '\033[93m'
                        elif event == 'PROGRESS': color = '\033[90m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, method, iterations,
                                       min_us, max_us, avg_us, stddev, prog, line])

                    except UnicodeDecodeError:
                        pass

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n[INFO] Interrupted")
    finally:
        if 'ser' in locals(): ser.close()

    print(f"\n{'='*70}")
    print(f"  SUMMARY - FreeRTOS Mechanism Benchmark")
    print(f"{'='*70}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Benchmarks started: {stats['bench_starts']}")
    print(f"  Benchmarks completed: {stats['bench_done']}")

    if bench_results:
        print(f"\n  {'Method':<25} {'Count':>8} {'Min(us)':>10} {'Max(us)':>10} {'Avg(us)':>10} {'StdDev':>10}")
        print(f"  {'-'*73}")
        for method, r in sorted(bench_results.items()):
            print(f"  {method:<25} {r['count']:>8} {r['min']:>10} {r['max']:>10} {r['avg']:>10} {r['stddev']:>10}")

    if stats['winners']:
        print(f"\n  Winners across runs:")
        for w, c in sorted(stats['winners'].items(), key=lambda x: -x[1]):
            print(f"    {w}: won {c} time(s)")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*70}")

if __name__ == '__main__':
    main()

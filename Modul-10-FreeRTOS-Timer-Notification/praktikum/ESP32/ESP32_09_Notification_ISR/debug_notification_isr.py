#!/usr/bin/env python3
"""
Debug Script: ESP32_09_Notification_ISR
=========================================
Monitors ISR-to-task notification latency comparison.
Tracks per-sample latency for both Notification and Semaphore methods.

Usage:
    python debug_notification_isr.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 120]
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
    parser = argparse.ArgumentParser(description='Notification ISR Latency Debug')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"notify_isr_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Notification ISR Latency Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"{'='*60}")

    stats = {'lines': 0, 'notify_samples': 0, 'sema_samples': 0, 'comparisons': 0}
    latencies = defaultdict(list)

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Press BOOT button. (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'method', 'press',
                           'latency_us', 'min_us', 'max_us', 'avg_us', 'tick', 'raw'])

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
                        press = ''
                        lat = ''
                        min_l = ''
                        max_l = ''
                        avg_l = ''
                        tick = ''

                        if 'NOTIFY_ISR' in line:
                            event = 'NOTIFY_SAMPLE'
                            method = 'NOTIFICATION'
                            stats['notify_samples'] += 1
                            m = re.search(r'latency=(\d+)', line)
                            if m: lat = m.group(1); latencies['notify'].append(int(lat))
                            m = re.search(r'press=(\d+)', line)
                            if m: press = m.group(1)
                            m = re.search(r'min=(\d+)', line)
                            if m: min_l = m.group(1)
                            m = re.search(r'max=(\d+)', line)
                            if m: max_l = m.group(1)
                            m = re.search(r'avg=(\d+)', line)
                            if m: avg_l = m.group(1)

                        elif 'SEMA_ISR' in line:
                            event = 'SEMA_SAMPLE'
                            method = 'SEMAPHORE'
                            stats['sema_samples'] += 1
                            m = re.search(r'latency=(\d+)', line)
                            if m: lat = m.group(1); latencies['sema'].append(int(lat))
                            m = re.search(r'press=(\d+)', line)
                            if m: press = m.group(1)
                            m = re.search(r'min=(\d+)', line)
                            if m: min_l = m.group(1)
                            m = re.search(r'max=(\d+)', line)
                            if m: max_l = m.group(1)
                            m = re.search(r'avg=(\d+)', line)
                            if m: avg_l = m.group(1)

                        elif 'COMPARE' in line:
                            event = 'COMPARE'
                            stats['comparisons'] += 1
                        elif 'SWITCH' in line:
                            event = 'SWITCH'
                        elif 'STATUS' in line:
                            event = 'STATUS'

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)

                        color = '\033[0m'
                        if event == 'NOTIFY_SAMPLE': color = '\033[92m'
                        elif event == 'SEMA_SAMPLE': color = '\033[96m'
                        elif event == 'COMPARE': color = '\033[95m'
                        elif event == 'SWITCH': color = '\033[93m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, method, press,
                                       lat, min_l, max_l, avg_l, tick, line])

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
    print(f"  SUMMARY - ISR Latency Comparison")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Notification samples: {stats['notify_samples']}")
    print(f"  Semaphore samples:    {stats['sema_samples']}")
    print(f"  Comparisons printed:  {stats['comparisons']}")

    for key, name in [('notify', 'Task Notification'), ('sema', 'Binary Semaphore')]:
        lats = latencies[key]
        if lats:
            avg = sum(lats) / len(lats)
            print(f"\n  {name} ISR Latency (us):")
            print(f"    Samples: {len(lats)}")
            print(f"    Avg: {avg:.1f} | Min: {min(lats)} | Max: {max(lats)}")
            # Percentiles
            sorted_lats = sorted(lats)
            p50 = sorted_lats[len(sorted_lats)//2]
            p95 = sorted_lats[int(len(sorted_lats)*0.95)] if len(sorted_lats) >= 20 else max(lats)
            print(f"    P50: {p50} | P95: {p95}")

    if latencies['notify'] and latencies['sema']:
        n_avg = sum(latencies['notify']) / len(latencies['notify'])
        s_avg = sum(latencies['sema']) / len(latencies['sema'])
        diff = s_avg - n_avg
        winner = 'Notification' if diff > 0 else 'Semaphore'
        print(f"\n  >> {winner} is faster by {abs(diff):.1f} us")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
Debug Script: ESP32_11_Event_Group_Sync
=========================================
Monitors barrier synchronization between 3 tasks.
Tracks sync counts, wait times, and verifies all tasks sync together.

Usage:
    python debug_event_group_sync.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
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
    parser = argparse.ArgumentParser(description='Event Group Sync Debug Monitor')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"evtgrp_sync_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Event Group Sync (Barrier) Debug Monitor")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud} | Output: {output_file}")
    print(f"  Tasks: Task_A (LED1), Task_B (LED2), Task_C (LED3)")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'work_starts': defaultdict(int), 'work_done': defaultdict(int),
        'barriers_reached': defaultdict(int), 'barriers_passed': defaultdict(int),
        'timeouts': 0, 'total_syncs': 0,
    }
    wait_times = defaultdict(list)
    barrier_reach_times = {}  # Per-cycle barrier reach timestamps

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. Listening... (Ctrl+C to stop)\n")
        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'task', 'task_id',
                           'work_time_ms', 'wait_us', 'sync_count', 'tick', 'raw'])

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
                        task = ''
                        task_id = ''
                        work_time = ''
                        wait_us = ''
                        sync_count = ''
                        tick = ''

                        if 'WORK_START' in line:
                            event = 'WORK_START'
                            m = re.search(r'task=(\w+)', line)
                            if m: task = m.group(1); stats['work_starts'][task] += 1
                            m = re.search(r'id=(\d+)', line)
                            if m: task_id = m.group(1)
                            m = re.search(r'work_time=(\d+)', line)
                            if m: work_time = m.group(1)

                        elif 'WORK_DONE' in line:
                            event = 'WORK_DONE'
                            m = re.search(r'task=(\w+)', line)
                            if m: task = m.group(1); stats['work_done'][task] += 1
                            m = re.search(r'id=(\d+)', line)
                            if m: task_id = m.group(1)

                        elif 'BARRIER_REACHED' in line:
                            event = 'BARRIER_REACHED'
                            m = re.search(r'task=(\w+)', line)
                            if m: task = m.group(1); stats['barriers_reached'][task] += 1
                            m = re.search(r'id=(\d+)', line)
                            if m: task_id = m.group(1)

                        elif 'BARRIER_PASSED' in line:
                            event = 'BARRIER_PASSED'
                            m = re.search(r'task=(\w+)', line)
                            if m: task = m.group(1); stats['barriers_passed'][task] += 1
                            m = re.search(r'id=(\d+)', line)
                            if m: task_id = m.group(1)
                            m = re.search(r'wait_us=(\d+)', line)
                            if m:
                                wait_us = m.group(1)
                                wait_times[task].append(int(wait_us))
                            m = re.search(r'sync_count=(\d+)', line)
                            if m: sync_count = m.group(1)

                        elif 'BARRIER_TIMEOUT' in line:
                            event = 'TIMEOUT'
                            stats['timeouts'] += 1
                            m = re.search(r'task=(\w+)', line)
                            if m: task = m.group(1)

                        elif 'STATUS' in line:
                            event = 'STATUS'
                            m = re.search(r'sync_count=(\d+)', line)
                            if m:
                                sync_count = m.group(1)
                                stats['total_syncs'] = int(sync_count)

                        m = re.search(r'tick=(\d+)', line)
                        if m: tick = m.group(1)

                        colors = {
                            'Task_A': '\033[92m', 'Task_B': '\033[93m', 'Task_C': '\033[96m'
                        }
                        color = colors.get(task, '\033[0m')
                        if event == 'TIMEOUT': color = '\033[91m'
                        elif event == 'BARRIER_PASSED': color = '\033[95m'

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, task, task_id,
                                       work_time, wait_us, sync_count, tick, line])

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
    print(f"  SUMMARY - Event Group Sync (Barrier)")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Total successful syncs: {stats['total_syncs']}")
    print(f"  Timeouts: {stats['timeouts']}")

    print(f"\n  Per-task statistics:")
    for task in ['Task_A', 'Task_B', 'Task_C']:
        print(f"    {task}:")
        print(f"      Work cycles: {stats['work_starts'].get(task, 0)}")
        print(f"      Barriers reached: {stats['barriers_reached'].get(task, 0)}")
        print(f"      Barriers passed:  {stats['barriers_passed'].get(task, 0)}")
        wt = wait_times.get(task, [])
        if wt:
            avg = sum(wt) / len(wt)
            print(f"      Barrier wait: avg={avg:.0f}us, min={min(wt)}us, max={max(wt)}us")

    # The task with shortest work time should have longest wait
    all_waits = {t: sum(w)/len(w) for t, w in wait_times.items() if w}
    if all_waits:
        print(f"\n  Average barrier wait times:")
        for t, w in sorted(all_waits.items(), key=lambda x: -x[1]):
            print(f"    {t}: {w:.0f} us (fastest worker waits longest)")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()

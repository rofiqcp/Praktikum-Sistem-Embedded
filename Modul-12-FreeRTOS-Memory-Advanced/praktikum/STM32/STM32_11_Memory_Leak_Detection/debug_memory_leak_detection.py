#!/usr/bin/env python3
"""
debug_memory_leak_detection.py
Tracks alloc/free pairs from STM32_11_Memory_Leak_Detection and identifies unmatched allocs.

Features:
- Parses TRACK_ALLOC and TRACK_FREE messages
- Identifies unmatched (leaked) allocations
- Shows allocation timeline
- Visualizes heap usage over time
- Generates leak report with task attribution
- Logs all data to CSV

Usage:
    python debug_memory_leak_detection.py --port /dev/ttyUSB0 --baud 115200
    python debug_memory_leak_detection.py --file log.txt
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime
from collections import defaultdict

import serial
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np


class MemoryLeakAnalyzer:
    def __init__(self):
        self.allocations = {}       # id -> {addr, size, task, tick, freed}
        self.free_events = []
        self.heap_snapshots = []    # [{tick, free, min_ever, alloc_count, free_count}]
        self.leak_reports = []
        self.detected_leaks = []
        self.raw_lines = []
        self.warnings = []

    def parse_line(self, line):
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # Parse TRACK_ALLOC
        m = re.match(r'\[TRACK_ALLOC\] id=(\d+) addr=0x([0-9A-Fa-f]+) size=(\d+) task=(\S+) tick=(\d+)', line)
        if m:
            alloc_id = int(m.group(1))
            addr = int(m.group(2), 16)
            size = int(m.group(3))
            task = m.group(4)
            tick = int(m.group(5))
            self.allocations[alloc_id] = {
                'addr': addr, 'size': size, 'task': task,
                'tick': tick, 'freed': False, 'time': time.time()
            }
            print(f"  ALLOC #{alloc_id}: {size}B @ 0x{addr:08X} by [{task}]")
            return

        # Parse TRACK_FREE
        m = re.match(r'\[TRACK_FREE\] id=(\d+) addr=0x([0-9A-Fa-f]+) size=(\d+) task=(\S+)', line)
        if m:
            alloc_id = int(m.group(1))
            addr = int(m.group(2), 16)
            size = int(m.group(3))
            task = m.group(4)
            if alloc_id in self.allocations:
                self.allocations[alloc_id]['freed'] = True
            self.free_events.append({
                'alloc_id': alloc_id, 'addr': addr, 'size': size,
                'task': task, 'time': time.time()
            })
            print(f"  FREE  #{alloc_id}: {size}B @ 0x{addr:08X} by [{task}]")
            return

        # Parse DETECTOR heap snapshots
        m = re.match(r'\[DETECTOR\] Heap free\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            self.heap_snapshots.append({
                'free': val, 'time': time.time()
            })
            print(f"  Heap free: {val} bytes")
            return

        m = re.match(r'\[DETECTOR\] Alloc calls\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            if self.heap_snapshots:
                self.heap_snapshots[-1]['alloc_calls'] = val
            return

        m = re.match(r'\[DETECTOR\] Free calls\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            if self.heap_snapshots:
                self.heap_snapshots[-1]['free_calls'] = val
            return

        m = re.match(r'\[DETECTOR\] Unmatched allocs:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            if self.heap_snapshots:
                self.heap_snapshots[-1]['unmatched'] = val
            print(f"  Unmatched allocs: {val}")
            return

        # Parse WARNING
        if '[DETECTOR] WARNING' in line:
            self.warnings.append(line)
            print(f"  *** {line} ***")
            return

        # Parse LEAK entries from report
        m = re.match(r'\[LEAK\] id=(\d+) addr=0x([0-9A-Fa-f]+) size=(\d+) task=(\S+) age=(\d+)', line)
        if m:
            leak = {
                'alloc_id': int(m.group(1)),
                'addr': int(m.group(2), 16),
                'size': int(m.group(3)),
                'task': m.group(4),
                'age_ms': int(m.group(5))
            }
            self.detected_leaks.append(leak)
            print(f"  LEAK: #{leak['alloc_id']} {leak['size']}B by [{leak['task']}] age={leak['age_ms']}ms")
            return

        # Parse LEAK_REPORT summary
        m = re.match(r'\[LEAK_REPORT\] (.+)', line)
        if m:
            content = m.group(1)
            if '===' not in content:
                print(f"  Report: {content}")
            return

        # Parse task messages
        for tag in ['GOOD_TASK', 'LEAKY_TASK']:
            if f'[{tag}]' in line:
                m2 = re.match(rf'\[{tag}\] (.+)', line)
                if m2:
                    print(f"  [{tag}] {m2.group(1)}")
                return

    def get_active_leaks(self):
        return {k: v for k, v in self.allocations.items() if not v['freed']}

    def save_csv(self, filename):
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            # All allocations
            writer.writerow(['Alloc_ID', 'Address', 'Size', 'Task', 'Tick', 'Freed', 'Status'])
            for aid, a in sorted(self.allocations.items()):
                status = 'OK' if a['freed'] else 'LEAKED'
                writer.writerow([aid, f"0x{a['addr']:08X}", a['size'],
                                 a['task'], a['tick'], a['freed'], status])

            writer.writerow([])
            writer.writerow(['Heap Snapshot', 'Free_Bytes', 'Alloc_Calls',
                             'Free_Calls', 'Unmatched'])
            for i, s in enumerate(self.heap_snapshots):
                writer.writerow([i, s.get('free', ''),
                                 s.get('alloc_calls', ''),
                                 s.get('free_calls', ''),
                                 s.get('unmatched', '')])

            writer.writerow([])
            writer.writerow(['Warnings'])
            for w in self.warnings:
                writer.writerow([w])
        print(f"\nData saved to {filename}")

    def plot_analysis(self):
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('STM32_11: Memory Leak Detection Analysis',
                     fontsize=14, fontweight='bold')

        # Plot 1: Allocation timeline
        ax1 = axes[0][0]
        if self.allocations:
            for aid, a in sorted(self.allocations.items()):
                color = '#2ecc71' if a['freed'] else '#e74c3c'
                marker = 'o' if a['freed'] else 'x'
                ax1.scatter(aid, a['size'], c=color, marker=marker, s=80, zorder=3)
                ax1.annotate(a['task'][:8], (aid, a['size']),
                             fontsize=6, ha='center', va='bottom')
            ok_patch = mpatches.Patch(color='#2ecc71', label='Freed (OK)')
            leak_patch = mpatches.Patch(color='#e74c3c', label='Leaked!')
            ax1.legend(handles=[ok_patch, leak_patch])
            ax1.set_xlabel('Allocation ID')
            ax1.set_ylabel('Size (bytes)')
            ax1.set_title('Allocation Timeline')
            ax1.grid(alpha=0.3)

        # Plot 2: Heap free over time
        ax2 = axes[0][1]
        if self.heap_snapshots:
            times = range(len(self.heap_snapshots))
            free_vals = [s.get('free', 0) for s in self.heap_snapshots]
            ax2.plot(times, free_vals, 'b-o', linewidth=2, markersize=6)
            ax2.fill_between(times, free_vals, alpha=0.2, color='blue')
            ax2.set_xlabel('Snapshot #')
            ax2.set_ylabel('Free Heap (bytes)')
            ax2.set_title('Heap Free Space Over Time')
            ax2.grid(alpha=0.3)
            # Show trend
            if len(free_vals) > 1:
                trend = free_vals[-1] - free_vals[0]
                trend_text = f"Trend: {'↓' if trend < 0 else '↑'} {abs(trend)} bytes"
                ax2.text(0.02, 0.95, trend_text, transform=ax2.transAxes,
                         fontsize=10, va='top',
                         color='red' if trend < 0 else 'green',
                         fontweight='bold')

        # Plot 3: Leaks by task
        ax3 = axes[1][0]
        leaks_by_task = defaultdict(lambda: {'count': 0, 'bytes': 0})
        for aid, a in self.allocations.items():
            if not a['freed']:
                leaks_by_task[a['task']]['count'] += 1
                leaks_by_task[a['task']]['bytes'] += a['size']
        if leaks_by_task:
            tasks = list(leaks_by_task.keys())
            counts = [leaks_by_task[t]['count'] for t in tasks]
            byte_vals = [leaks_by_task[t]['bytes'] for t in tasks]
            x = range(len(tasks))
            width = 0.35
            bars1 = ax3.bar([i - width / 2 for i in x], counts, width,
                           label='Leak Count', color='#e74c3c')
            ax3_twin = ax3.twinx()
            bars2 = ax3_twin.bar([i + width / 2 for i in x], byte_vals, width,
                                label='Leaked Bytes', color='#e67e22', alpha=0.7)
            ax3.set_xticks(list(x))
            ax3.set_xticklabels(tasks)
            ax3.set_ylabel('Leak Count', color='#e74c3c')
            ax3_twin.set_ylabel('Leaked Bytes', color='#e67e22')
            ax3.set_title('Leaks by Task')
            lines1, labels1 = ax3.get_legend_handles_labels()
            lines2, labels2 = ax3_twin.get_legend_handles_labels()
            ax3.legend(lines1 + lines2, labels1 + labels2, loc='upper left')
        else:
            ax3.text(0.5, 0.5, 'No Leaks Detected!', transform=ax3.transAxes,
                     ha='center', va='center', fontsize=16, color='green',
                     fontweight='bold')
            ax3.set_title('Leaks by Task')

        # Plot 4: Alloc vs Free count
        ax4 = axes[1][1]
        if self.heap_snapshots:
            alloc_counts = [s.get('alloc_calls', 0) for s in self.heap_snapshots]
            free_counts = [s.get('free_calls', 0) for s in self.heap_snapshots]
            unmatched = [s.get('unmatched', 0) for s in self.heap_snapshots]
            x = range(len(self.heap_snapshots))
            ax4.plot(x, alloc_counts, 'r-o', label='Alloc calls', markersize=5)
            ax4.plot(x, free_counts, 'g-s', label='Free calls', markersize=5)
            ax4.plot(x, unmatched, 'm-^', label='Unmatched (leaks)', markersize=6,
                     linewidth=2)
            ax4.set_xlabel('Snapshot #')
            ax4.set_ylabel('Count')
            ax4.set_title('Alloc vs Free Call Counts')
            ax4.legend()
            ax4.grid(alpha=0.3)

        plt.tight_layout()
        plt.savefig('memory_leak_analysis.png', dpi=150, bbox_inches='tight')
        print("\nPlot saved to memory_leak_analysis.png")
        plt.show()


def monitor_serial(port, baud, duration=60):
    analyzer = MemoryLeakAnalyzer()
    print(f"Connecting to {port} at {baud} baud...")
    print(f"Monitoring for {duration} seconds...\n")

    try:
        ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        start_time = time.time()
        while (time.time() - start_time) < duration:
            if ser.in_waiting:
                try:
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        analyzer.parse_line(line)
                except Exception as e:
                    print(f"Parse error: {e}")
        ser.close()
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return None

    return analyzer


def process_file(filename):
    analyzer = MemoryLeakAnalyzer()
    print(f"Processing log file: {filename}\n")
    with open(filename, 'r') as f:
        for line in f:
            analyzer.parse_line(line)
    return analyzer


def main():
    parser = argparse.ArgumentParser(description='Memory Leak Detection Analyzer')
    parser.add_argument('--port', type=str, default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--duration', type=int, default=60,
                        help='Monitoring duration in seconds (default: 60)')
    parser.add_argument('--file', type=str, default=None,
                        help='Process log file instead of serial')
    parser.add_argument('--csv', type=str, default='memory_leak_log.csv',
                        help='CSV output filename')
    parser.add_argument('--no-plot', action='store_true',
                        help='Skip plotting')

    args = parser.parse_args()

    if args.file:
        analyzer = process_file(args.file)
    else:
        analyzer = monitor_serial(args.port, args.baud, args.duration)

    if analyzer:
        analyzer.save_csv(args.csv)

        # Print final summary
        active_leaks = analyzer.get_active_leaks()
        print("\n" + "=" * 60)
        print("MEMORY LEAK ANALYSIS SUMMARY")
        print("=" * 60)
        print(f"  Total allocations tracked : {len(analyzer.allocations)}")
        print(f"  Freed properly            : {sum(1 for a in analyzer.allocations.values() if a['freed'])}")
        print(f"  LEAKED (unfreed)          : {len(active_leaks)}")
        if active_leaks:
            total_leaked = sum(a['size'] for a in active_leaks.values())
            print(f"  Total leaked bytes        : {total_leaked}")
            print(f"\n  Leaked allocations:")
            for aid, a in sorted(active_leaks.items()):
                print(f"    #{aid}: {a['size']}B @ 0x{a['addr']:08X} by [{a['task']}]")
        print(f"  Heap warnings             : {len(analyzer.warnings)}")

        if not args.no_plot:
            analyzer.plot_analysis()


if __name__ == '__main__':
    main()

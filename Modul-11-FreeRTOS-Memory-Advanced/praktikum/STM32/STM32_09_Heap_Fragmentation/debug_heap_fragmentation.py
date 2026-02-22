#!/usr/bin/env python3
"""
debug_heap_fragmentation.py
Visualizes heap fragmentation patterns from STM32_09_Heap_Fragmentation.

Features:
- Parses ALLOC/FREE/HEAP_STATS messages from serial output
- Visualizes memory block map as a bar chart
- Shows fragmentation progression over phases
- Logs all data to CSV
- Real-time display mode

Usage:
    python debug_heap_fragmentation.py --port /dev/ttyUSB0 --baud 115200
    python debug_heap_fragmentation.py --file log.txt
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime
from collections import OrderedDict

import serial
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.animation as animation
import numpy as np


class HeapFragmentationAnalyzer:
    def __init__(self):
        self.blocks = OrderedDict()
        self.heap_stats_history = []
        self.alloc_events = []
        self.free_events = []
        self.phases = []
        self.current_phase = "Init"
        self.raw_lines = []

    def parse_line(self, line):
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # Parse ALLOC events
        m = re.match(r'\[ALLOC\] Block #(\d+): addr=0x([0-9A-Fa-f]+) size=(\d+)', line)
        if m:
            idx = int(m.group(1))
            addr = int(m.group(2), 16)
            size = int(m.group(3))
            self.blocks[idx] = {'addr': addr, 'size': size, 'active': True}
            self.alloc_events.append({'index': idx, 'addr': addr, 'size': size,
                                      'phase': self.current_phase, 'time': time.time()})
            print(f"  [ALLOC] Block #{idx}: 0x{addr:08X} ({size} bytes)")
            return

        # Parse ALLOC_FAIL events
        m = re.match(r'\[ALLOC_FAIL\] Could not allocate (\d+) bytes', line)
        if m:
            size = int(m.group(1))
            print(f"  [ALLOC_FAIL] Failed to allocate {size} bytes!")
            return

        # Parse FREE events
        m = re.match(r'\[FREE\] Block #(\d+): addr=0x([0-9A-Fa-f]+) size=(\d+)', line)
        if m:
            idx = int(m.group(1))
            addr = int(m.group(2), 16)
            size = int(m.group(3))
            if idx in self.blocks:
                self.blocks[idx]['active'] = False
            self.free_events.append({'index': idx, 'addr': addr, 'size': size,
                                     'phase': self.current_phase, 'time': time.time()})
            print(f"  [FREE] Block #{idx}: 0x{addr:08X} ({size} bytes)")
            return

        # Parse HEAP_STATS
        m = re.match(r'\[HEAP_STATS\] (.+)', line)
        if m:
            label = m.group(1)
            self.current_phase = label
            self.phases.append(label)
            print(f"\n=== HEAP STATS: {label} ===")
            return

        # Parse stats values
        m = re.match(r'\s+Free bytes\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            self.heap_stats_history.append({'phase': self.current_phase,
                                            'free_bytes': val})
            print(f"  Free bytes: {val}")
            return

        m = re.match(r'\s+Largest free blk\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            if self.heap_stats_history:
                self.heap_stats_history[-1]['largest_free'] = val
            print(f"  Largest free block: {val}")
            return

        m = re.match(r'\s+Free blocks\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            if self.heap_stats_history:
                self.heap_stats_history[-1]['free_blocks'] = val
            print(f"  Free blocks count: {val}")
            return

        m = re.match(r'\s+Min ever free\s*:\s*(\d+)', line)
        if m:
            val = int(m.group(1))
            if self.heap_stats_history:
                self.heap_stats_history[-1]['min_ever_free'] = val
            print(f"  Min ever free: {val}")
            return

        # Parse FRAGMENTATION result
        if '[FRAGMENTATION]' in line:
            print(f"\n  *** {line} ***")
            return

        # Parse ANALYSIS
        m = re.match(r'\[ANALYSIS\] (.+)', line)
        if m:
            print(f"  [ANALYSIS] {m.group(1)}")
            return

        # Parse phase headers
        if '--- PHASE' in line:
            print(f"\n{'='*50}")
            print(f"  {line}")
            print(f"{'='*50}")
            return

        # Print other relevant lines
        if any(tag in line for tag in ['[ATTEMPT]', '[SUCCESS]', '[FAIL]',
                                        '[LESSON]', '[DONE]', '[NOTE]']):
            print(f"  {line}")

    def save_csv(self, filename):
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Timestamp', 'Event', 'Block_Index', 'Address',
                             'Size', 'Phase'])
            for evt in self.alloc_events:
                writer.writerow([evt['time'], 'ALLOC', evt['index'],
                                 f"0x{evt['addr']:08X}", evt['size'], evt['phase']])
            for evt in self.free_events:
                writer.writerow([evt['time'], 'FREE', evt['index'],
                                 f"0x{evt['addr']:08X}", evt['size'], evt['phase']])
            writer.writerow([])
            writer.writerow(['Phase', 'Free_Bytes', 'Largest_Free',
                             'Free_Blocks', 'Min_Ever_Free'])
            for stat in self.heap_stats_history:
                writer.writerow([stat.get('phase', ''),
                                 stat.get('free_bytes', ''),
                                 stat.get('largest_free', ''),
                                 stat.get('free_blocks', ''),
                                 stat.get('min_ever_free', '')])
        print(f"\nData saved to {filename}")

    def plot_fragmentation(self):
        fig, axes = plt.subplots(3, 1, figsize=(14, 12))
        fig.suptitle('STM32_09: Heap Fragmentation Analysis', fontsize=14, fontweight='bold')

        # Plot 1: Memory block map
        ax1 = axes[0]
        if self.blocks:
            sorted_blocks = sorted(self.blocks.items(), key=lambda x: x[1]['addr'])
            y_pos = 0
            for idx, blk in sorted_blocks:
                color = '#2ecc71' if blk['active'] else '#e74c3c'
                label_text = f"#{idx} ({blk['size']}B)"
                bar = ax1.barh(y_pos, blk['size'], left=blk['addr'],
                               height=0.8, color=color, edgecolor='black', linewidth=0.5)
                ax1.text(blk['addr'] + blk['size'] / 2, y_pos, label_text,
                         ha='center', va='center', fontsize=7, fontweight='bold')
                y_pos += 1
            active_patch = mpatches.Patch(color='#2ecc71', label='Allocated (USED)')
            freed_patch = mpatches.Patch(color='#e74c3c', label='Freed (GAP)')
            ax1.legend(handles=[active_patch, freed_patch], loc='upper right')
        ax1.set_xlabel('Memory Address')
        ax1.set_ylabel('Block Index')
        ax1.set_title('Memory Block Map (Final State)')

        # Plot 2: Heap stats over phases
        ax2 = axes[1]
        if self.heap_stats_history:
            phases_labels = [s.get('phase', '')[:20] for s in self.heap_stats_history]
            free_bytes = [s.get('free_bytes', 0) for s in self.heap_stats_history]
            largest_free = [s.get('largest_free', 0) for s in self.heap_stats_history]
            x = range(len(phases_labels))
            width = 0.35
            ax2.bar([i - width / 2 for i in x], free_bytes, width,
                    label='Total Free', color='#3498db')
            ax2.bar([i + width / 2 for i in x], largest_free, width,
                    label='Largest Free Block', color='#e67e22')
            ax2.set_xticks(list(x))
            ax2.set_xticklabels(phases_labels, rotation=30, ha='right', fontsize=8)
            ax2.set_ylabel('Bytes')
            ax2.set_title('Heap Free Space Progression')
            ax2.legend()
            ax2.grid(axis='y', alpha=0.3)

        # Plot 3: Fragmentation index
        ax3 = axes[2]
        if self.heap_stats_history:
            frag_index = []
            labels = []
            for s in self.heap_stats_history:
                total_free = s.get('free_bytes', 1)
                largest = s.get('largest_free', 0)
                if total_free > 0:
                    frag = 1.0 - (largest / total_free)
                else:
                    frag = 0.0
                frag_index.append(frag * 100)
                labels.append(s.get('phase', '')[:20])
            colors = ['#2ecc71' if f < 20 else '#f39c12' if f < 50 else '#e74c3c'
                       for f in frag_index]
            ax3.bar(range(len(frag_index)), frag_index, color=colors, edgecolor='black')
            ax3.set_xticks(range(len(labels)))
            ax3.set_xticklabels(labels, rotation=30, ha='right', fontsize=8)
            ax3.set_ylabel('Fragmentation Index (%)')
            ax3.set_title('Fragmentation Index (0%=none, 100%=severe)')
            ax3.set_ylim(0, 105)
            ax3.axhline(y=50, color='red', linestyle='--', alpha=0.5, label='Warning threshold')
            ax3.legend()
            ax3.grid(axis='y', alpha=0.3)

        plt.tight_layout()
        plt.savefig('heap_fragmentation_analysis.png', dpi=150, bbox_inches='tight')
        print("\nPlot saved to heap_fragmentation_analysis.png")
        plt.show()


def monitor_serial(port, baud, duration=30):
    analyzer = HeapFragmentationAnalyzer()
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
    analyzer = HeapFragmentationAnalyzer()
    print(f"Processing log file: {filename}\n")

    with open(filename, 'r') as f:
        for line in f:
            analyzer.parse_line(line)

    return analyzer


def main():
    parser = argparse.ArgumentParser(description='Heap Fragmentation Debug Analyzer')
    parser.add_argument('--port', type=str, default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--duration', type=int, default=30,
                        help='Monitoring duration in seconds (default: 30)')
    parser.add_argument('--file', type=str, default=None,
                        help='Process log file instead of serial')
    parser.add_argument('--csv', type=str, default='heap_fragmentation_log.csv',
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
        if not args.no_plot:
            analyzer.plot_fragmentation()

        # Print summary
        print("\n" + "=" * 50)
        print("SUMMARY")
        print("=" * 50)
        print(f"  Total allocations : {len(analyzer.alloc_events)}")
        print(f"  Total frees       : {len(analyzer.free_events)}")
        print(f"  Phases recorded   : {len(analyzer.heap_stats_history)}")
        active = sum(1 for b in analyzer.blocks.values() if b['active'])
        print(f"  Final active blks : {active}")


if __name__ == '__main__':
    main()

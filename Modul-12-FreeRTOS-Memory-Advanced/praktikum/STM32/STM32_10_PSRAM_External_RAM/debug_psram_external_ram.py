#!/usr/bin/env python3
"""
debug_psram_external_ram.py
Parses memory addresses from STM32_10_PSRAM_External_RAM and visualizes memory map.

Features:
- Parses REGION/MEMMAP/SIZE/SPEED messages from serial output
- Visualizes STM32F103 memory map with variable placements
- Shows memory region usage as stacked bar chart
- Speed comparison chart
- Logs all data to CSV

Usage:
    python debug_psram_external_ram.py --port /dev/ttyUSB0 --baud 115200
    python debug_psram_external_ram.py --file log.txt
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime

import serial
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np


class MemoryMapAnalyzer:
    def __init__(self):
        self.regions = []
        self.variables = []
        self.sizes = {}
        self.speed_results = []
        self.heap_info = {}
        self.raw_lines = []

    def parse_line(self, line):
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # Parse REGION entries
        m = re.match(r'\[REGION\]\s+(\w[\w\.\[\]_]*)\s*:\s*addr=0x([0-9A-Fa-f]+)\s+region=(.+?)(?:\s+size=(\d+))?$', line)
        if m:
            name = m.group(1)
            addr = int(m.group(2), 16)
            region = m.group(3).strip()
            size = int(m.group(4)) if m.group(4) else 0
            self.variables.append({'name': name, 'addr': addr, 'region': region, 'size': size})
            print(f"  Variable: {name:20s} @ 0x{addr:08X} [{region}]")
            return

        # Parse section header REGION lines
        m = re.match(r'\[REGION\]\s+(.+)', line)
        if m:
            print(f"  {m.group(1)}")
            return

        # Parse SIZE entries
        m = re.match(r'\[SIZE\]\s+(.+?)\s*:\s*(?:0x([0-9A-Fa-f]+)|(\d+)\s*bytes)', line)
        if m:
            label = m.group(1).strip()
            if m.group(2):
                val = int(m.group(2), 16)
                self.sizes[label] = val
                print(f"  Size: {label:30s} = 0x{val:08X}")
            elif m.group(3):
                val = int(m.group(3))
                self.sizes[label] = val
                print(f"  Size: {label:30s} = {val} bytes")
            return

        # Parse SPEED entries
        m = re.match(r'\[SPEED\]\s+(\S.+?)\s*:\s*(\d+)\s*ms', line)
        if m:
            label = m.group(1).strip()
            ms = int(m.group(2))
            self.speed_results.append({'label': label, 'time_ms': ms})
            print(f"  Speed: {label:35s} = {ms} ms")
            return

        # Parse HEAP entries
        m = re.match(r'\[HEAP\]\s+(.+?)\s*:\s*(\d+)\s*bytes', line)
        if m:
            label = m.group(1).strip()
            val = int(m.group(2))
            self.heap_info[label] = val
            print(f"  Heap: {label:30s} = {val} bytes")
            return

        # Parse MEMMAP
        m = re.match(r'\[MEMMAP\]\s+(.+)', line)
        if m:
            print(f"  MemMap: {m.group(1)}")
            return

        # Parse EXTRAM
        m = re.match(r'\[EXTRAM\]\s+(.+)', line)
        if m:
            print(f"  ExtRAM: {m.group(1)}")
            return

        # Parse CONCEPT
        m = re.match(r'\[CONCEPT\]\s+(.+)', line)
        if m:
            print(f"  Concept: {m.group(1)}")
            return

        if '[DONE]' in line:
            print(f"\n  {line}")

    def save_csv(self, filename):
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Variable', 'Address', 'Region', 'Size'])
            for v in self.variables:
                writer.writerow([v['name'], f"0x{v['addr']:08X}", v['region'], v['size']])
            writer.writerow([])
            writer.writerow(['Section', 'Value'])
            for k, v in self.sizes.items():
                writer.writerow([k, v])
            writer.writerow([])
            writer.writerow(['Speed Test', 'Time (ms)'])
            for s in self.speed_results:
                writer.writerow([s['label'], s['time_ms']])
            writer.writerow([])
            writer.writerow(['Heap Info', 'Bytes'])
            for k, v in self.heap_info.items():
                writer.writerow([k, v])
        print(f"\nData saved to {filename}")

    def plot_memory_map(self):
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('STM32_10: Memory Region Analysis (PSRAM/External RAM Concept)',
                     fontsize=14, fontweight='bold')

        # Plot 1: Memory map with variable positions
        ax1 = axes[0][0]
        regions = [
            ('Flash', 0x08000000, 64 * 1024, '#3498db'),
            ('SRAM', 0x20000000, 20 * 1024, '#2ecc71'),
            ('Peripherals', 0x40000000, 512 * 1024, '#95a5a6'),
            ('FSMC (N/A)', 0x60000000, 256 * 1024, '#e74c3c'),
        ]
        for i, (name, base, size, color) in enumerate(regions):
            ax1.barh(i, 1, color=color, edgecolor='black', alpha=0.7)
            ax1.text(0.5, i, f"{name}\n0x{base:08X}\n({size // 1024}KB)",
                     ha='center', va='center', fontsize=8, fontweight='bold')
        # Mark variables
        var_y_offset = len(regions) + 0.5
        for j, v in enumerate(self.variables):
            color = '#e67e22'
            if 'FLASH' in v['region'] or 'text' in v['region']:
                color = '#3498db'
            elif 'SRAM' in v['region']:
                color = '#2ecc71'
            ax1.barh(var_y_offset + j * 0.4, 0.5, height=0.35,
                     color=color, edgecolor='black', alpha=0.8)
            ax1.text(0.55, var_y_offset + j * 0.4,
                     f"{v['name']} @ 0x{v['addr']:08X}",
                     ha='left', va='center', fontsize=7)
        ax1.set_xlim(-0.1, 2)
        ax1.set_title('STM32F103C8 Memory Regions & Variables')
        ax1.set_xlabel('')
        ax1.get_xaxis().set_visible(False)
        ax1.set_yticks([])

        # Plot 2: Variable addresses sorted
        ax2 = axes[0][1]
        if self.variables:
            sorted_vars = sorted(self.variables, key=lambda x: x['addr'])
            names = [v['name'] for v in sorted_vars]
            addrs = [v['addr'] for v in sorted_vars]
            colors = []
            for v in sorted_vars:
                if v['addr'] >= 0x08000000 and v['addr'] < 0x08010000:
                    colors.append('#3498db')
                elif v['addr'] >= 0x20000000 and v['addr'] < 0x20005000:
                    colors.append('#2ecc71')
                else:
                    colors.append('#95a5a6')
            y_pos = range(len(names))
            ax2.barh(y_pos, addrs, color=colors, edgecolor='black', height=0.6)
            ax2.set_yticks(list(y_pos))
            ax2.set_yticklabels(names, fontsize=7)
            ax2.set_xlabel('Address')
            ax2.set_title('Variable Addresses (Sorted)')
            flash_patch = mpatches.Patch(color='#3498db', label='Flash')
            sram_patch = mpatches.Patch(color='#2ecc71', label='SRAM')
            ax2.legend(handles=[flash_patch, sram_patch], fontsize=8)

        # Plot 3: Speed comparison
        ax3 = axes[1][0]
        if self.speed_results:
            labels = [s['label'][:25] for s in self.speed_results]
            times = [s['time_ms'] for s in self.speed_results]
            bars = ax3.bar(range(len(labels)), times,
                          color=['#3498db', '#2ecc71', '#e67e22', '#9b59b6'],
                          edgecolor='black')
            ax3.set_xticks(range(len(labels)))
            ax3.set_xticklabels(labels, rotation=30, ha='right', fontsize=8)
            ax3.set_ylabel('Time (ms)')
            ax3.set_title('Memory Access Speed Comparison')
            ax3.grid(axis='y', alpha=0.3)
            for bar, t in zip(bars, times):
                ax3.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                         f'{t}ms', ha='center', va='bottom', fontsize=9)

        # Plot 4: Memory usage breakdown
        ax4 = axes[1][1]
        data_size = self.sizes.get('.data size', 0)
        bss_size = self.sizes.get('.bss size', 0)
        heap_total = 15 * 1024
        heap_used = heap_total - self.heap_info.get('Available', heap_total)
        sram_total = 20 * 1024
        stack_est = sram_total - data_size - bss_size - heap_total
        if stack_est < 0:
            stack_est = 1024

        sizes_plot = [data_size, bss_size, heap_used, heap_total - heap_used, stack_est]
        labels_plot = [f'.data ({data_size}B)', f'.bss ({bss_size}B)',
                       f'Heap used ({heap_used}B)', f'Heap free ({heap_total - heap_used}B)',
                       f'Stack (~{stack_est}B)']
        colors_plot = ['#e74c3c', '#e67e22', '#2ecc71', '#a8e6cf', '#3498db']
        valid = [(s, l, c) for s, l, c in zip(sizes_plot, labels_plot, colors_plot) if s > 0]
        if valid:
            s_v, l_v, c_v = zip(*valid)
            ax4.pie(s_v, labels=l_v, colors=c_v, autopct='%1.1f%%',
                    startangle=90, textprops={'fontsize': 8})
            ax4.set_title(f'SRAM Usage Breakdown ({sram_total // 1024}KB total)')

        plt.tight_layout()
        plt.savefig('psram_memory_analysis.png', dpi=150, bbox_inches='tight')
        print("\nPlot saved to psram_memory_analysis.png")
        plt.show()


def monitor_serial(port, baud, duration=30):
    analyzer = MemoryMapAnalyzer()
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
    analyzer = MemoryMapAnalyzer()
    print(f"Processing log file: {filename}\n")
    with open(filename, 'r') as f:
        for line in f:
            analyzer.parse_line(line)
    return analyzer


def main():
    parser = argparse.ArgumentParser(description='PSRAM/External RAM Memory Map Analyzer')
    parser.add_argument('--port', type=str, default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--duration', type=int, default=30,
                        help='Monitoring duration in seconds (default: 30)')
    parser.add_argument('--file', type=str, default=None,
                        help='Process log file instead of serial')
    parser.add_argument('--csv', type=str, default='psram_memory_log.csv',
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
            analyzer.plot_memory_map()

        print("\n" + "=" * 50)
        print("SUMMARY")
        print("=" * 50)
        print(f"  Variables found  : {len(analyzer.variables)}")
        print(f"  Speed tests      : {len(analyzer.speed_results)}")
        for v in analyzer.variables:
            print(f"    {v['name']:20s} @ 0x{v['addr']:08X} [{v['region']}]")


if __name__ == '__main__':
    main()

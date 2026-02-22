#!/usr/bin/env python3
"""
debug_memory_pool.py - Memory Pool Allocator Debug & Analysis Tool

Tracks block allocation/release events from STM32_05_Memory_Pool.
Shows pool utilization over time with matplotlib plotting.
Logs data to CSV for post-analysis.

Usage:
    python debug_memory_pool.py [--port /dev/ttyUSB0] [--baud 115200] [--csv log.csv]
"""

import serial
import re
import sys
import argparse
import csv
import time
from datetime import datetime
from collections import deque

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not available, plotting disabled")


class MemoryPoolMonitor:
    def __init__(self, port, baud, csv_file=None):
        self.port = port
        self.baud = baud
        self.csv_file = csv_file
        self.csv_writer = None
        self.csv_fh = None

        # Data storage
        self.timestamps = deque(maxlen=500)
        self.free_blocks = deque(maxlen=500)
        self.used_blocks = deque(maxlen=500)
        self.utilization = deque(maxlen=500)
        self.alloc_events = deque(maxlen=200)
        self.free_events = deque(maxlen=200)

        # Counters
        self.total_allocs = 0
        self.total_frees = 0
        self.alloc_fails = 0
        self.corruption_warnings = 0
        self.heap_remaining = 0
        self.start_time = time.time()

        # Regex patterns
        self.re_pool_event = re.compile(
            r'\[POOL\] Task(\d+) cycle(\d+): alloc=(\d+)/(\d+) free=(\d+) used=(\d+)')
        self.re_consumer = re.compile(
            r'\[POOL\] Consumer(\d+): wrote \[(.+?)\]')
        self.re_consumer_fail = re.compile(
            r'\[POOL\] Consumer(\d+): alloc FAILED')
        self.re_warning = re.compile(
            r'\[POOL\] WARNING: Task(\d+) block(\d+) corrupted')
        self.re_stats_free = re.compile(r'\[STATS\] Free blocks\s*:\s*(\d+)')
        self.re_stats_used = re.compile(r'\[STATS\] Used blocks\s*:\s*(\d+)')
        self.re_stats_util = re.compile(r'\[STATS\] Utilization\s*:\s*(\d+)%')
        self.re_stats_allocs = re.compile(r'\[STATS\] Total allocs\s*:\s*(\d+)')
        self.re_stats_frees = re.compile(r'\[STATS\] Total frees\s*:\s*(\d+)')
        self.re_stats_fails = re.compile(r'\[STATS\] Alloc fails\s*:\s*(\d+)')
        self.re_stats_heap = re.compile(r'\[STATS\] Heap remain\s*:\s*(\d+)')

    def init_csv(self):
        if self.csv_file:
            self.csv_fh = open(self.csv_file, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_fh)
            self.csv_writer.writerow([
                'timestamp', 'elapsed_s', 'event_type', 'task_id',
                'alloc_got', 'alloc_requested', 'free_blocks', 'used_blocks',
                'total_allocs', 'total_frees', 'alloc_fails', 'heap_remaining'
            ])

    def log_csv(self, event_type, task_id=0, alloc_got=0, alloc_req=0,
                free_blk=0, used_blk=0):
        if self.csv_writer:
            elapsed = time.time() - self.start_time
            self.csv_writer.writerow([
                datetime.now().isoformat(), f'{elapsed:.3f}',
                event_type, task_id, alloc_got, alloc_req,
                free_blk, used_blk, self.total_allocs, self.total_frees,
                self.alloc_fails, self.heap_remaining
            ])
            self.csv_fh.flush()

    def parse_line(self, line):
        elapsed = time.time() - self.start_time

        # Pool allocation event
        m = self.re_pool_event.search(line)
        if m:
            task_id = int(m.group(1))
            cycle = int(m.group(2))
            got = int(m.group(3))
            requested = int(m.group(4))
            free = int(m.group(5))
            used = int(m.group(6))

            self.timestamps.append(elapsed)
            self.free_blocks.append(free)
            self.used_blocks.append(used)
            self.utilization.append(used * 100 / 8)
            self.alloc_events.append((elapsed, task_id, got, requested))

            self.log_csv('alloc', task_id, got, requested, free, used)
            print(f"  [{elapsed:8.2f}s] Task{task_id} cycle{cycle}: "
                  f"alloc {got}/{requested}, pool: {used}/8 used ({used*100//8}%)")
            return

        # Consumer write
        m = self.re_consumer.search(line)
        if m:
            task_id = int(m.group(1))
            msg = m.group(2)
            print(f"  [{elapsed:8.2f}s] Consumer{task_id}: '{msg}'")
            self.log_csv('consumer_write', task_id)
            return

        # Consumer fail
        m = self.re_consumer_fail.search(line)
        if m:
            task_id = int(m.group(1))
            print(f"  [{elapsed:8.2f}s] *** Consumer{task_id}: ALLOC FAILED ***")
            self.log_csv('alloc_fail', task_id)
            return

        # Corruption warning
        m = self.re_warning.search(line)
        if m:
            self.corruption_warnings += 1
            task_id = int(m.group(1))
            block = int(m.group(2))
            print(f"  [{elapsed:8.2f}s] !!! CORRUPTION: Task{task_id} block{block} !!!")
            self.log_csv('corruption', task_id)
            return

        # Stats parsing
        m = self.re_stats_free.search(line)
        if m:
            free = int(m.group(1))
            self.timestamps.append(elapsed)
            self.free_blocks.append(free)
            self.used_blocks.append(8 - free)
            self.utilization.append((8 - free) * 100 / 8)

        m = self.re_stats_allocs.search(line)
        if m:
            self.total_allocs = int(m.group(1))

        m = self.re_stats_frees.search(line)
        if m:
            self.total_frees = int(m.group(1))

        m = self.re_stats_fails.search(line)
        if m:
            self.alloc_fails = int(m.group(1))

        m = self.re_stats_heap.search(line)
        if m:
            self.heap_remaining = int(m.group(1))
            self.log_csv('stats', 0, 0, 0,
                         self.free_blocks[-1] if self.free_blocks else 0,
                         self.used_blocks[-1] if self.used_blocks else 0)

    def print_summary(self):
        elapsed = time.time() - self.start_time
        print(f"\n{'='*50}")
        print(f"  MEMORY POOL MONITOR SUMMARY")
        print(f"{'='*50}")
        print(f"  Duration       : {elapsed:.1f} seconds")
        print(f"  Total allocs   : {self.total_allocs}")
        print(f"  Total frees    : {self.total_frees}")
        print(f"  Alloc failures : {self.alloc_fails}")
        print(f"  Corruptions    : {self.corruption_warnings}")
        print(f"  Heap remaining : {self.heap_remaining} bytes")
        if self.utilization:
            avg_util = sum(self.utilization) / len(self.utilization)
            max_util = max(self.utilization)
            print(f"  Avg utilization: {avg_util:.1f}%")
            print(f"  Max utilization: {max_util:.1f}%")
        print(f"{'='*50}\n")

    def run_realtime(self):
        self.init_csv()
        print(f"\n[*] Connecting to {self.port} @ {self.baud} baud...")

        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            sys.exit(1)

        print(f"[*] Connected. Monitoring memory pool...")
        print(f"[*] Press Ctrl+C to stop\n")

        try:
            while True:
                raw = ser.readline()
                if raw:
                    try:
                        line = raw.decode('utf-8', errors='replace').strip()
                    except Exception:
                        continue
                    if line:
                        self.parse_line(line)
        except KeyboardInterrupt:
            print("\n[*] Stopped by user")
        finally:
            self.print_summary()
            ser.close()
            if self.csv_fh:
                self.csv_fh.close()
                print(f"[*] CSV saved to: {self.csv_file}")

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] Cannot plot - matplotlib not installed")
            return

        if not self.timestamps:
            print("[WARN] No data to plot")
            return

        fig, axes = plt.subplots(3, 1, figsize=(12, 9), sharex=True)
        fig.suptitle('STM32 Memory Pool Monitor', fontsize=14, fontweight='bold')

        ts = list(self.timestamps)

        # Pool usage
        axes[0].fill_between(ts, list(self.used_blocks), alpha=0.3, color='red',
                             label='Used')
        axes[0].fill_between(ts, list(self.free_blocks), alpha=0.3, color='green',
                             label='Free')
        axes[0].plot(ts, list(self.used_blocks), 'r-', linewidth=1.5)
        axes[0].plot(ts, list(self.free_blocks), 'g-', linewidth=1.5)
        axes[0].set_ylabel('Blocks')
        axes[0].set_title('Pool Block Usage')
        axes[0].set_ylim(0, 9)
        axes[0].axhline(y=8, color='gray', linestyle='--', alpha=0.5,
                        label='Total (8)')
        axes[0].legend(loc='upper right')
        axes[0].grid(True, alpha=0.3)

        # Utilization
        axes[1].plot(ts, list(self.utilization), 'b-', linewidth=1.5)
        axes[1].fill_between(ts, list(self.utilization), alpha=0.2, color='blue')
        axes[1].set_ylabel('Utilization (%)')
        axes[1].set_title('Pool Utilization')
        axes[1].set_ylim(0, 105)
        axes[1].axhline(y=75, color='orange', linestyle='--', alpha=0.5,
                        label='Warning (75%)')
        axes[1].axhline(y=100, color='red', linestyle='--', alpha=0.5,
                        label='Full (100%)')
        axes[1].legend(loc='upper right')
        axes[1].grid(True, alpha=0.3)

        # Allocation events scatter
        if self.alloc_events:
            ae_t = [e[0] for e in self.alloc_events]
            ae_got = [e[2] for e in self.alloc_events]
            ae_req = [e[3] for e in self.alloc_events]
            ae_task = [e[1] for e in self.alloc_events]

            colors = ['blue' if t == 1 else 'orange' for t in ae_task]
            axes[2].scatter(ae_t, ae_got, c=colors, s=30, alpha=0.7,
                           label='Blocks obtained')
            axes[2].scatter(ae_t, ae_req, c=colors, s=15, alpha=0.3,
                           marker='x', label='Blocks requested')
            axes[2].set_ylabel('Blocks')
            axes[2].set_title('Allocation Events by Task')
            axes[2].legend(loc='upper right')
            axes[2].grid(True, alpha=0.3)

        axes[2].set_xlabel('Time (seconds)')
        plt.tight_layout()
        plt.savefig('memory_pool_analysis.png', dpi=150, bbox_inches='tight')
        print("[*] Plot saved to: memory_pool_analysis.png")
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='STM32 Memory Pool Debug Monitor')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--csv', '-c', default='memory_pool_log.csv',
                        help='CSV output file (default: memory_pool_log.csv)')
    parser.add_argument('--plot', action='store_true',
                        help='Show plot after monitoring')
    args = parser.parse_args()

    monitor = MemoryPoolMonitor(args.port, args.baud, args.csv)
    monitor.run_realtime()

    if args.plot:
        monitor.plot_results()


if __name__ == '__main__':
    main()

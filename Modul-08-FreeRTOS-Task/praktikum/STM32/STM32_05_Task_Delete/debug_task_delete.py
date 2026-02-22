#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_05_Task_Delete
==========================================================================
 Mem-parse output serial dari program Task Create/Delete FreeRTOS.
 Mengekstrak event pembuatan/penghapusan task dan penggunaan heap.

 Penggunaan:
   python debug_task_delete.py --port /dev/ttyUSB0
   python debug_task_delete.py --file capture.log
   python debug_task_delete.py --demo

 Output:
   - Grafik heap usage over time
   - Timeline task lifecycle (create/delete)
   - Deteksi memory leak
==========================================================================
"""

import argparse
import sys
import re
import time
import os
from datetime import datetime
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class TaskDeleteParser:
    """Parser dan visualizer untuk data Task Create/Delete."""

    def __init__(self):
        self.heap_data = []        # [(time, free_heap, min_ever)]
        self.task_events = []      # [(time, event, detail)]  event: 'create'|'delete'|'fail'
        self.counter_data = []     # [(time, counter_value)]
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()
        self.task_alive = False
        self.initial_heap = None

    def parse_line(self, line):
        self.total_lines += 1
        line = line.strip()
        if not line:
            return
        t = time.time() - self.start_time
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

        # Parse heap status: "Free Heap: XXXX bytes, Min Ever: YYYY bytes"
        m = re.search(r'Free Heap:\s*(\d+)\s*bytes.*Min Ever:\s*(\d+)\s*bytes', line)
        if m:
            free_heap = int(m.group(1))
            min_ever = int(m.group(2))
            self.heap_data.append((t, free_heap, min_ever))
            if self.initial_heap is None:
                self.initial_heap = free_heap
            self.data_lines += 1
            print(f"[{ts}] 📊 Heap: {free_heap} bytes free, min ever: {min_ever}")
            return

        # Parse task creation
        if 'Creating Dynamic Task' in line:
            self.task_events.append((t, 'create', line))
            self.data_lines += 1
            print(f"[{ts}] 🟢 CREATING task...")
            return

        if 'Created Successfully' in line:
            self.task_alive = True
            self.task_events.append((t, 'created', line))
            self.data_lines += 1
            print(f"[{ts}] ✅ Task created successfully")
            return

        if 'Failed to Create' in line:
            self.task_events.append((t, 'fail', line))
            self.data_lines += 1
            print(f"[{ts}] ❌ Task creation FAILED!")
            return

        # Parse task deletion
        if 'Deleting Dynamic Task' in line:
            self.task_events.append((t, 'delete', line))
            self.data_lines += 1
            print(f"[{ts}] 🔴 DELETING task...")
            return

        if 'Task Deleted' in line:
            self.task_alive = False
            self.task_events.append((t, 'deleted', line))
            self.data_lines += 1
            print(f"[{ts}] 🗑️  Task deleted")
            return

        # Parse dynamic task counter
        m2 = re.search(r'Dynamic Task Counter:\s*(\d+)', line)
        if m2:
            cnt = int(m2.group(1))
            self.counter_data.append((t, cnt))
            if cnt % 50 == 0:
                print(f"[{ts}]    DynTask counter: {cnt}")
            return

        # Parse [DATA] tags if present
        dm = re.match(r'\[DATA\]\s*(\w+),(.*)', line)
        if dm:
            tag = dm.group(1)
            self.data_lines += 1
            print(f"[{ts}] 📌 [{tag}] {dm.group(2)}")
            return

        # Generic
        if 'Main Task' in line or 'Dynamic Task Running' in line:
            print(f"[{ts}] ℹ️  {line}")

    def print_summary(self):
        creates = sum(1 for _, e, _ in self.task_events if e in ('create', 'created'))
        deletes = sum(1 for _, e, _ in self.task_events if e in ('delete', 'deleted'))
        fails = sum(1 for _, e, _ in self.task_events if e == 'fail')

        print(f"\n{'='*55}")
        print(f" RINGKASAN TASK DELETE")
        print(f"{'='*55}")
        print(f"  Total lines parsed : {self.total_lines}")
        print(f"  Data lines         : {self.data_lines}")
        print(f"  Task creates       : {creates // 2 if creates > 1 else creates}")
        print(f"  Task deletes       : {deletes // 2 if deletes > 1 else deletes}")
        print(f"  Create failures    : {fails}")
        print(f"  Heap samples       : {len(self.heap_data)}")
        if self.heap_data:
            heaps = [h[1] for h in self.heap_data]
            print(f"  Heap range         : {min(heaps)} - {max(heaps)} bytes")
            if self.initial_heap and heaps[-1] < self.initial_heap:
                leak = self.initial_heap - heaps[-1]
                print(f"  ⚠️  Possible leak   : {leak} bytes not recovered")
            else:
                print(f"  ✅ No memory leak detected")
        print(f"{'='*55}")

    def plot(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] matplotlib tidak tersedia, skip plot.")
            return
        if not self.heap_data and not self.task_events:
            print("[WARN] Tidak ada data untuk di-plot.")
            return

        fig, axes = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
        fig.suptitle('STM32_05 Task Create/Delete Analysis', fontsize=14)

        # Plot 1: Heap usage over time
        ax1 = axes[0]
        if self.heap_data:
            times = [h[0] for h in self.heap_data]
            free_heaps = [h[1] for h in self.heap_data]
            min_evers = [h[2] for h in self.heap_data]
            ax1.plot(times, free_heaps, 'b-o', markersize=4, label='Free Heap', linewidth=2)
            ax1.plot(times, min_evers, 'r--', markersize=3, label='Min Ever', linewidth=1)
            ax1.fill_between(times, free_heaps, alpha=0.15, color='blue')
        # Mark create/delete events
        for t, etype, _ in self.task_events:
            if etype == 'create':
                ax1.axvline(x=t, color='green', linestyle='--', alpha=0.6, linewidth=1)
            elif etype in ('delete', 'deleted'):
                ax1.axvline(x=t, color='red', linestyle='--', alpha=0.6, linewidth=1)
        ax1.set_ylabel('Bytes')
        ax1.set_title('Heap Usage Over Time')
        ax1.grid(True, alpha=0.3)
        ax1.legend(loc='upper right', fontsize=8)

        # Plot 2: Task lifecycle
        ax2 = axes[1]
        state_times = []
        state_vals = []
        current = 0
        for t, etype, _ in self.task_events:
            if etype in ('create', 'created'):
                current = 1
            elif etype in ('delete', 'deleted'):
                current = 0
            elif etype == 'fail':
                current = -1
            state_times.append(t)
            state_vals.append(current)
        if state_times:
            ax2.step(state_times, state_vals, where='post', linewidth=2, color='#FF9800')
            ax2.fill_between(state_times, state_vals, step='post', alpha=0.2, color='#FF9800')
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('State')
        ax2.set_yticks([-1, 0, 1])
        ax2.set_yticklabels(['Failed', 'Deleted', 'Alive'])
        ax2.set_title('Dynamic Task Lifecycle')
        ax2.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('task_delete_analysis.png', dpi=150)
        print("[INFO] Plot disimpan: task_delete_analysis.png")
        plt.show()


def generate_demo_data():
    """Generate simulated task create/delete data."""
    lines = []
    lines.append("=== Task Delete Demo ===")
    lines.append("Main Task Started")
    lines.append("Free Heap: 5280 bytes, Min Ever: 5280 bytes")

    heap = 5280
    counter = 0
    for i in range(6):
        # Create
        lines.append("")
        lines.append("--- Creating Dynamic Task ---")
        heap -= 820
        lines.append("Dynamic Task Created Successfully!")
        lines.append(f"Free Heap: {heap} bytes, Min Ever: {heap} bytes")
        # Run for a while
        for j in range(1, 31):
            counter += 1
            lines.append(f"Dynamic Task Counter: {counter}")
        # Delete
        lines.append("")
        lines.append("--- Deleting Dynamic Task ---")
        heap += 820
        lines.append("Dynamic Task Deleted!")
        lines.append(f"Free Heap: {heap} bytes, Min Ever: {heap - 820} bytes")
    # One failure at the end
    lines.append("")
    lines.append("--- Creating Dynamic Task ---")
    lines.append("Failed to Create Task!")
    lines.append(f"Free Heap: {heap} bytes, Min Ever: {heap - 820} bytes")
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug parser untuk STM32_05 Task Create/Delete')
    parser.add_argument('--port', type=str, help='Serial port (e.g. /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', type=str, help='File log untuk di-parse')
    parser.add_argument('--demo', action='store_true', help='Jalankan dengan data demo')
    args = parser.parse_args()

    p = TaskDeleteParser()

    if args.demo:
        print("[INFO] Mode DEMO - menggunakan data simulasi\n")
        for line in generate_demo_data():
            p.parse_line(line)
            time.sleep(0.01)
        p.print_summary()
        p.plot()
    elif args.file:
        print(f"[INFO] Membaca file: {args.file}\n")
        with open(args.file, 'r', errors='replace') as f:
            for line in f:
                p.parse_line(line)
        p.print_summary()
        p.plot()
    elif args.port:
        if not HAS_SERIAL:
            print("[ERROR] pyserial belum terinstall. pip install pyserial")
            sys.exit(1)
        print(f"[INFO] Membuka {args.port} @ {args.baud} baud\n")
        try:
            ser = serial.Serial(args.port, args.baud, timeout=1)
            while True:
                raw = ser.readline()
                if raw:
                    p.parse_line(raw.decode('utf-8', errors='replace'))
        except KeyboardInterrupt:
            print("\n[INFO] Dihentikan oleh user.")
            ser.close()
            p.print_summary()
            p.plot()
        except serial.SerialException as e:
            print(f"[ERROR] Serial: {e}")
            sys.exit(1)
    else:
        parser.print_help()
        print("\nContoh: python debug_task_delete.py --demo")


if __name__ == '__main__':
    main()

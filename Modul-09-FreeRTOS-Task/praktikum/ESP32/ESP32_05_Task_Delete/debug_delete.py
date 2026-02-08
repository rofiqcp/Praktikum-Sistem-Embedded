#!/usr/bin/env python3
"""
Debug Serial Parser & Visualizer - ESP32_05_Task_Delete
Mem-parse output [DATA] dan menampilkan visualisasi
task lifecycle, heap usage, dan memory leak detection.

Usage:
    python debug_delete.py --port /dev/ttyUSB0
    python debug_delete.py --file capture.log
"""

import sys
import argparse
import time
from collections import defaultdict

try:
    import serial
except ImportError:
    serial = None

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    from matplotlib.gridspec import GridSpec
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia")


class TaskDeleteParser:
    """Parser untuk data serial ESP32 Task Delete"""

    def __init__(self):
        self.heap_data = []         # (ts, free, min_free, num_tasks, creates, deletes)
        self.worker_events = []     # (ts, action, data...)
        self.selfdelete_events = [] # (ts, action, task_id, ...)
        self.cycle_data = []        # (ts, action, cycle, ...)
        self.created_data = []      # (ts, count, heap, cost)
        self.deleted_data = []      # (ts, count, heap, recovered, original_cost)
        self.leak_warnings = []     # (ts, allocated, recovered)
        self.leak_trends = []       # (ts, trend_bytes)
        self.init_info = {}

    def parse_line(self, line):
        line = line.strip()
        if not line.startswith("[DATA]"):
            return

        parts = line.replace("[DATA] ", "").split(",")
        if len(parts) < 2:
            return

        tag = parts[0]
        try:
            if tag == "INIT" and len(parts) >= 3:
                self.init_info = {
                    'timestamp': int(parts[1]),
                    'start_heap': int(parts[2])
                }
                print(f"[INIT] Start heap: {parts[2]} bytes")

            elif tag == "HEAP" and len(parts) >= 7:
                ts = int(parts[1])
                free_h = int(parts[2])
                min_h = int(parts[3])
                n_tasks = int(parts[4])
                creates = int(parts[5])
                deletes = int(parts[6])
                self.heap_data.append((ts, free_h, min_h, n_tasks, creates, deletes))

            elif tag == "WORKER" and len(parts) >= 4:
                action = parts[1]
                ts = int(parts[2])
                data = parts[3] if len(parts) > 3 else ''
                extra = parts[4] if len(parts) > 4 else ''
                self.worker_events.append((ts, action, data, extra))

            elif tag == "SELFDELETE" and len(parts) >= 4:
                action = parts[1]
                ts = int(parts[2])
                task_id = parts[3] if len(parts) > 3 else '0'
                extra = parts[4] if len(parts) > 4 else ''
                extra2 = parts[5] if len(parts) > 5 else ''
                self.selfdelete_events.append((ts, action, task_id, extra, extra2))

            elif tag == "CYCLE" and len(parts) >= 4:
                action = parts[1]
                ts = int(parts[2])
                cycle = int(parts[3])
                rest = parts[4:] if len(parts) > 4 else []
                self.cycle_data.append((ts, action, cycle, rest))

            elif tag == "CREATED" and len(parts) >= 5:
                ts = int(parts[1])
                count = int(parts[2])
                heap = int(parts[3])
                cost = int(parts[4])
                self.created_data.append((ts, count, heap, cost))
                print(f"  [CREATED] #{count}, cost={cost} bytes")

            elif tag == "DELETED" and len(parts) >= 6:
                ts = int(parts[1])
                count = int(parts[2])
                heap = int(parts[3])
                recovered = int(parts[4])
                cost = int(parts[5])
                self.deleted_data.append((ts, count, heap, recovered, cost))
                print(f"  [DELETED] #{count}, recovered={recovered} bytes")

            elif tag == "LEAK_WARN" and len(parts) >= 4:
                ts = int(parts[1])
                alloc = int(parts[2])
                recov = int(parts[3])
                self.leak_warnings.append((ts, alloc, recov))
                print(f"  [LEAK] allocated={alloc}, recovered={recov}")

            elif tag == "LEAK_TREND" and len(parts) >= 3:
                ts = int(parts[1])
                trend = int(parts[2])
                self.leak_trends.append((ts, trend))

        except (ValueError, IndexError):
            pass

    def print_summary(self):
        print("\n" + "=" * 60)
        print("RINGKASAN TASK DELETE DATA")
        print("=" * 60)
        print(f"Heap samples: {len(self.heap_data)}")
        print(f"Worker events: {len(self.worker_events)}")
        print(f"Self-delete events: {len(self.selfdelete_events)}")
        print(f"Create events: {len(self.created_data)}")
        print(f"Delete events: {len(self.deleted_data)}")
        print(f"Leak warnings: {len(self.leak_warnings)}")

        if self.created_data:
            costs = [c[3] for c in self.created_data]
            print(f"\nTask memory cost: avg={sum(costs)//len(costs)}, "
                  f"min={min(costs)}, max={max(costs)} bytes")

        if self.deleted_data:
            recovs = [d[3] for d in self.deleted_data]
            print(f"Memory recovered: avg={sum(recovs)//len(recovs)}, "
                  f"min={min(recovs)}, max={max(recovs)} bytes")

        if self.heap_data:
            heaps = [h[1] for h in self.heap_data]
            start_heap = self.init_info.get('start_heap', heaps[0])
            print(f"\nHeap: start={start_heap}, end={heaps[-1]}, "
                  f"min={min(heaps)}, max={max(heaps)}")
            net_change = heaps[-1] - start_heap
            print(f"Net heap change: {net_change} bytes "
                  f"({'leak!' if net_change < -100 else 'OK'})")

        print("=" * 60)

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            return

        fig = plt.figure(figsize=(15, 12))
        fig.suptitle("ESP32 Task Delete & Memory Management Analysis",
                     fontsize=14, fontweight='bold')
        gs = GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.3)

        # 1. Heap Usage Over Time
        ax1 = fig.add_subplot(gs[0, :])
        if self.heap_data:
            ts = [d[0] / 1000.0 for d in self.heap_data]
            free_h = [d[1] / 1024.0 for d in self.heap_data]
            min_h = [d[2] / 1024.0 for d in self.heap_data]
            ax1.plot(ts, free_h, 'g-', label='Free Heap', linewidth=2)
            ax1.plot(ts, min_h, 'r--', label='Min Free', linewidth=1)
            ax1.fill_between(ts, min_h, free_h, alpha=0.15, color='green')

        # Mark create/delete events
        for c in self.created_data:
            ax1.axvline(x=c[0] / 1000.0, color='blue', linestyle=':', alpha=0.5)
        for d in self.deleted_data:
            ax1.axvline(x=d[0] / 1000.0, color='red', linestyle=':', alpha=0.5)
        for l in self.leak_warnings:
            ax1.axvline(x=l[0] / 1000.0, color='orange', linestyle='-', alpha=0.7,
                       linewidth=2)

        ax1.set_xlabel("Waktu (detik)")
        ax1.set_ylabel("Heap (KB)")
        ax1.set_title("Heap Memory (blue=create, red=delete, orange=leak)")
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # 2. Task Count Over Time
        ax2 = fig.add_subplot(gs[1, 0])
        if self.heap_data:
            ts = [d[0] / 1000.0 for d in self.heap_data]
            n_tasks = [d[3] for d in self.heap_data]
            ax2.step(ts, n_tasks, where='post', color='#2196F3', linewidth=2)
        ax2.set_xlabel("Waktu (detik)")
        ax2.set_ylabel("Jumlah Task")
        ax2.set_title("Active Task Count")
        ax2.grid(True, alpha=0.3)

        # 3. Memory Cost per Creation
        ax3 = fig.add_subplot(gs[1, 1])
        if self.created_data:
            indices = range(len(self.created_data))
            costs = [c[3] / 1024.0 for c in self.created_data]
            ax3.bar(indices, costs, color='#42A5F5', alpha=0.7, label='Allocated')
        if self.deleted_data:
            indices = range(len(self.deleted_data))
            recovs = [abs(d[3]) / 1024.0 for d in self.deleted_data]
            ax3.bar(indices, recovs, color='#66BB6A', alpha=0.5, label='Recovered')
        ax3.set_xlabel("Cycle #")
        ax3.set_ylabel("Memory (KB)")
        ax3.set_title("Memory Allocated vs Recovered per Cycle")
        ax3.legend()
        ax3.grid(True, alpha=0.3, axis='y')

        # 4. Create/Delete Cumulative Count
        ax4 = fig.add_subplot(gs[2, 0])
        if self.heap_data:
            ts = [d[0] / 1000.0 for d in self.heap_data]
            creates = [d[4] for d in self.heap_data]
            deletes = [d[5] for d in self.heap_data]
            ax4.plot(ts, creates, 'b-', label='Created', linewidth=2)
            ax4.plot(ts, deletes, 'r-', label='Deleted', linewidth=2)
        ax4.set_xlabel("Waktu (detik)")
        ax4.set_ylabel("Cumulative Count")
        ax4.set_title("Task Create/Delete Count")
        ax4.legend()
        ax4.grid(True, alpha=0.3)

        # 5. Self-Delete Task Timeline
        ax5 = fig.add_subplot(gs[2, 1])
        sd_starts = [(e[0], int(e[2])) for e in self.selfdelete_events if e[1] == 'START']
        sd_dones = [(e[0], int(e[2])) for e in self.selfdelete_events if e[1] == 'DONE']

        if sd_starts:
            for ts, tid in sd_starts:
                ax5.barh(tid, 0.1, left=ts / 1000.0, color='blue', height=0.3)
        if sd_dones:
            for ts, tid in sd_dones:
                ax5.barh(tid, 0.1, left=ts / 1000.0, color='green', height=0.3)

        # Draw duration bars
        for start in sd_starts:
            matching_done = [d for d in sd_dones if d[1] == start[1]]
            if matching_done:
                duration = (matching_done[0][0] - start[0]) / 1000.0
                ax5.barh(start[1], duration, left=start[0] / 1000.0,
                        color='#90CAF9', height=0.3, alpha=0.5)

        ax5.set_xlabel("Waktu (detik)")
        ax5.set_ylabel("Task ID")
        ax5.set_title("Self-Deleting Task Lifecycle")
        ax5.grid(True, alpha=0.3)

        plt.savefig("debug_delete.png", dpi=150, bbox_inches='tight')
        print("[INFO] Grafik disimpan: debug_delete.png")
        plt.show()


def read_serial(port, baudrate, parser, duration=60):
    if serial is None:
        print("[ERROR] pyserial tidak terinstall")
        return
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[INFO] Connected to {port}.")
        start = time.time()
        while (time.time() - start) < duration:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore')
                print(line.rstrip())
                parser.parse_line(line)
        ser.close()
    except Exception as e:
        print(f"[ERROR] {e}")


def read_file(filepath, parser):
    try:
        with open(filepath, 'r') as f:
            for line in f:
                parser.parse_line(line)
    except FileNotFoundError:
        print(f"[ERROR] File tidak ditemukan: {filepath}")


def main():
    argp = argparse.ArgumentParser(description="Debug parser ESP32 Task Delete")
    argp.add_argument('--port', type=str, default=None)
    argp.add_argument('--baud', type=int, default=115200)
    argp.add_argument('--file', type=str, default=None)
    argp.add_argument('--duration', type=int, default=90)
    args = argp.parse_args()

    parser = TaskDeleteParser()

    if args.file:
        read_file(args.file, parser)
    elif args.port:
        read_serial(args.port, args.baud, parser, args.duration)
    else:
        print("[INFO] Membaca dari stdin...")
        try:
            for line in sys.stdin:
                print(line.rstrip())
                parser.parse_line(line)
        except KeyboardInterrupt:
            pass

    parser.print_summary()
    parser.plot_results()


if __name__ == "__main__":
    main()

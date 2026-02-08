#!/usr/bin/env python3
"""
Debug Serial Parser & Visualizer - ESP32_06_Task_Stack_Monitor
Mem-parse output [DATA] dan menampilkan visualisasi
stack usage, high water mark, dan warning thresholds.

Usage:
    python debug_stack.py --port /dev/ttyUSB0
    python debug_stack.py --file capture.log
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


class StackMonitorParser:
    """Parser untuk data serial ESP32 Stack Monitor"""

    def __init__(self):
        self.stack_data = defaultdict(list)  # {task_name: [(ts, total, hwm, used, pct, result)]}
        self.stack_reports = defaultdict(list)  # {task_name: [(ts, total, used, free, pct, status, max_pct)]}
        self.stack_warnings = []             # (ts, task_name, pct)
        self.overflow_events = []            # (ts, task_name)
        self.system_data = []                # (ts, heap, shallow_c, medium_c, deep_c)
        self.heap_data = []                  # (ts, free_heap, min_heap)
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
            if tag == "INIT" and len(parts) >= 9:
                self.init_info = {
                    'timestamp': int(parts[1]),
                    'small_stack': int(parts[2]),
                    'medium_stack': int(parts[3]),
                    'large_stack': int(parts[4]),
                    'shallow_depth': int(parts[5]),
                    'medium_depth': int(parts[6]),
                    'deep_depth': int(parts[7]),
                    'warn_pct': int(parts[8])
                }
                print(f"[INIT] Stacks: {parts[2]}/{parts[3]}/{parts[4]} bytes, "
                      f"Depths: {parts[5]}/{parts[6]}/{parts[7]}")

            elif tag == "STACK" and len(parts) >= 7:
                name = parts[1]
                ts = int(parts[2])
                total = int(parts[3])
                hwm = int(parts[4])
                used = int(parts[5])
                pct = float(parts[6])
                result = int(parts[7]) if len(parts) > 7 else 0
                self.stack_data[name].append((ts, total, hwm, used, pct, result))

            elif tag == "STACK_REPORT" and len(parts) >= 9:
                name = parts[1]
                ts = int(parts[2])
                total = int(parts[3])
                used = int(parts[4])
                free = int(parts[5])
                pct = float(parts[6])
                status = parts[7]
                max_pct = int(parts[8])
                self.stack_reports[name].append((ts, total, used, free, pct, status, max_pct))

            elif tag == "STACK_WARN" and len(parts) >= 4:
                name = parts[1]
                ts = int(parts[2])
                pct = float(parts[3])
                self.stack_warnings.append((ts, name, pct))
                print(f"  [WARN] {name} stack at {pct:.1f}%!")

            elif tag == "STACK_OVERFLOW" and len(parts) >= 3:
                ts = int(parts[1])
                name = parts[2]
                self.overflow_events.append((ts, name))
                print(f"  [OVERFLOW] {name} STACK OVERFLOW!")

            elif tag == "SYSTEM" and len(parts) >= 6:
                ts = int(parts[1])
                heap = int(parts[2])
                sc = int(parts[3])
                mc = int(parts[4])
                dc = int(parts[5])
                self.system_data.append((ts, heap, sc, mc, dc))

            elif tag == "HEAP" and len(parts) >= 4:
                ts = int(parts[1])
                free_h = int(parts[2])
                min_h = int(parts[3])
                self.heap_data.append((ts, free_h, min_h))

        except (ValueError, IndexError):
            pass

    def print_summary(self):
        print("\n" + "=" * 60)
        print("RINGKASAN STACK MONITOR DATA")
        print("=" * 60)

        for name in ['Shallow', 'Medium', 'Deep']:
            data = self.stack_data.get(name, [])
            reports = self.stack_reports.get(name, [])

            if data:
                pcts = [d[4] for d in data]
                print(f"\n{name} Task:")
                print(f"  Samples: {len(data)}")
                print(f"  Stack size: {data[0][1]} bytes")
                print(f"  Usage: min={min(pcts):.1f}%, max={max(pcts):.1f}%, "
                      f"avg={sum(pcts)/len(pcts):.1f}%")
                print(f"  Free (HWM): {data[-1][2]} bytes")
                print(f"  Used: {data[-1][3]} bytes")

            if reports:
                last = reports[-1]
                print(f"  Status: {last[5]}")
                print(f"  Max usage ever: {last[6]}%")

        warnings_by_task = defaultdict(int)
        for _, name, _ in self.stack_warnings:
            warnings_by_task[name] += 1

        print(f"\nTotal warnings: {len(self.stack_warnings)}")
        for name, count in warnings_by_task.items():
            print(f"  {name}: {count} warnings")

        print(f"Stack overflows: {len(self.overflow_events)}")
        print("=" * 60)

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            return

        fig = plt.figure(figsize=(15, 13))
        fig.suptitle("ESP32 FreeRTOS Stack Usage Monitor",
                     fontsize=14, fontweight='bold')
        gs = GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.3)

        colors = {'Shallow': '#4CAF50', 'Medium': '#FF9800', 'Deep': '#F44336'}
        warn_pct = self.init_info.get('warn_pct', 80)

        # 1. Stack Usage Percentage Over Time
        ax1 = fig.add_subplot(gs[0, :])
        for name in ['Shallow', 'Medium', 'Deep']:
            data = self.stack_data.get(name, [])
            if data:
                ts = [d[0] / 1000.0 for d in data]
                pct = [d[4] for d in data]
                ax1.plot(ts, pct, '-o', label=f'{name} ({data[0][1]}B)',
                        color=colors[name], markersize=3, linewidth=2)
        ax1.axhline(y=warn_pct, color='red', linestyle='--', alpha=0.7,
                   label=f'Warning ({warn_pct}%)')
        ax1.axhline(y=100, color='darkred', linestyle='-', alpha=0.5,
                   label='Overflow (100%)')
        ax1.set_xlabel("Waktu (detik)")
        ax1.set_ylabel("Stack Usage (%)")
        ax1.set_title("Stack Usage Over Time")
        ax1.legend(loc='upper left')
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(0, 110)

        # 2. Stack Breakdown (Stacked Bar)
        ax2 = fig.add_subplot(gs[1, 0])
        names = []
        used_vals = []
        free_vals = []
        total_vals = []
        for name in ['Shallow', 'Medium', 'Deep']:
            data = self.stack_data.get(name, [])
            if data:
                last = data[-1]
                names.append(name)
                used_vals.append(last[3] / 1024.0)
                free_vals.append(last[2] / 1024.0)
                total_vals.append(last[1] / 1024.0)

        if names:
            x = range(len(names))
            ax2.bar(x, used_vals, label='Used', color='#EF5350', alpha=0.8)
            ax2.bar(x, free_vals, bottom=used_vals, label='Free (HWM)',
                   color='#66BB6A', alpha=0.8)
            ax2.set_xticks(list(x))
            ax2.set_xticklabels(names)
            # Add text labels
            for i, (u, f, t) in enumerate(zip(used_vals, free_vals, total_vals)):
                ax2.text(i, u / 2, f'{u:.1f}KB', ha='center', va='center',
                        fontsize=9, fontweight='bold', color='white')
                ax2.text(i, u + f / 2, f'{f:.1f}KB', ha='center', va='center',
                        fontsize=9)
        ax2.set_ylabel("Stack (KB)")
        ax2.set_title("Stack Breakdown (Used vs Free)")
        ax2.legend()
        ax2.grid(True, alpha=0.3, axis='y')

        # 3. High Water Mark Over Time
        ax3 = fig.add_subplot(gs[1, 1])
        for name in ['Shallow', 'Medium', 'Deep']:
            data = self.stack_data.get(name, [])
            if data:
                ts = [d[0] / 1000.0 for d in data]
                hwm = [d[2] for d in data]
                ax3.plot(ts, hwm, '-', label=f'{name}',
                        color=colors[name], linewidth=2)
        ax3.set_xlabel("Waktu (detik)")
        ax3.set_ylabel("Free Stack (bytes)")
        ax3.set_title("Stack High Water Mark (Free Space)")
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # 4. Warning Events
        ax4 = fig.add_subplot(gs[2, 0])
        if self.stack_warnings:
            for name in ['Shallow', 'Medium', 'Deep']:
                warns = [(w[0] / 1000.0, w[2]) for w in self.stack_warnings
                         if w[1] == name]
                if warns:
                    ts = [w[0] for w in warns]
                    pcts = [w[1] for w in warns]
                    ax4.scatter(ts, pcts, label=name, color=colors[name],
                              s=50, zorder=5, edgecolors='black', linewidth=0.5)
        ax4.axhline(y=warn_pct, color='red', linestyle='--', alpha=0.5)
        ax4.set_xlabel("Waktu (detik)")
        ax4.set_ylabel("Usage %")
        ax4.set_title("Stack Warning Events")
        ax4.legend()
        ax4.grid(True, alpha=0.3)

        # 5. Heap + Task Cycles
        ax5 = fig.add_subplot(gs[2, 1])
        if self.system_data:
            ts = [d[0] / 1000.0 for d in self.system_data]
            heap = [d[1] / 1024.0 for d in self.system_data]
            ax5.plot(ts, heap, 'g-', label='Free Heap', linewidth=2)
            ax5_twin = ax5.twinx()
            ax5_twin.plot(ts, [d[2] for d in self.system_data], ':',
                         color=colors['Shallow'], label='Shallow cycles', alpha=0.7)
            ax5_twin.plot(ts, [d[3] for d in self.system_data], ':',
                         color=colors['Medium'], label='Medium cycles', alpha=0.7)
            ax5_twin.plot(ts, [d[4] for d in self.system_data], ':',
                         color=colors['Deep'], label='Deep cycles', alpha=0.7)
            ax5_twin.set_ylabel("Task Cycles")
            ax5_twin.legend(loc='upper left', fontsize=8)
        ax5.set_xlabel("Waktu (detik)")
        ax5.set_ylabel("Heap (KB)")
        ax5.set_title("Heap & Task Activity")
        ax5.legend(loc='upper right')
        ax5.grid(True, alpha=0.3)

        plt.savefig("debug_stack.png", dpi=150, bbox_inches='tight')
        print("[INFO] Grafik disimpan: debug_stack.png")
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
    argp = argparse.ArgumentParser(description="Debug parser ESP32 Stack Monitor")
    argp.add_argument('--port', type=str, default=None)
    argp.add_argument('--baud', type=int, default=115200)
    argp.add_argument('--file', type=str, default=None)
    argp.add_argument('--duration', type=int, default=60)
    args = argp.parse_args()

    parser = StackMonitorParser()

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

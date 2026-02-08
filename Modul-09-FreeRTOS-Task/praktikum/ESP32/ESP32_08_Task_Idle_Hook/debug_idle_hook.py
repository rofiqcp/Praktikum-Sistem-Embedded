#!/usr/bin/env python3
"""
============================================================================
Debug Parser & Visualizer - ESP32 Task Idle Hook & CPU Usage
============================================================================
Mem-parse output serial [DATA] dari program ESP32_08_Task_Idle_Hook
dan menampilkan grafik CPU usage dual-core secara real-time.

Fitur:
  - Grafik CPU usage per core (line chart real-time)
  - Bar chart CPU usage saat ini
  - Grafik idle counter over time
  - Info beban dan heap
  - Visual ASCII bar CPU dari serial

Penggunaan:
  python debug_idle_hook.py --port /dev/ttyUSB0
  python debug_idle_hook.py --file output.log
============================================================================
"""

import argparse
import sys
import re
import time
import threading
from collections import deque

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.gridspec import GridSpec
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class IdleHookParser:
    """Parser untuk data serial ESP32 Idle Hook"""

    def __init__(self, max_samples=300):
        self.max_samples = max_samples
        # CPU usage history
        self.cpu0_history = deque(maxlen=max_samples)
        self.cpu1_history = deque(maxlen=max_samples)
        self.avg0_history = deque(maxlen=max_samples)
        self.avg1_history = deque(maxlen=max_samples)
        self.timestamps = deque(maxlen=max_samples)
        # Current values
        self.cpu0_current = 0.0
        self.cpu1_current = 0.0
        self.avg0_current = 0.0
        self.avg1_current = 0.0
        # Idle counters
        self.idle_core0 = 0
        self.idle_core1 = 0
        self.idle_history_c0 = deque(maxlen=max_samples)
        self.idle_history_c1 = deque(maxlen=max_samples)
        # Load info
        self.load_level = 0
        self.load_percent = 0
        self.load_changes = []
        # Heap info
        self.heap_free = 0
        self.heap_min = 0
        # Calibration
        self.max_idle_core0 = 0
        self.max_idle_core1 = 0
        # Counters
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial"""
        self.total_lines += 1
        line = line.strip()

        if not line.startswith("[DATA]"):
            return

        self.data_lines += 1
        content = line[6:]
        elapsed = time.time() - self.start_time

        # CPU_USAGE
        m = re.match(r'CPU_USAGE,core0=([\d.]+),core1=([\d.]+),avg0=([\d.]+),avg1=([\d.]+)', content)
        if m:
            self.cpu0_current = float(m.group(1))
            self.cpu1_current = float(m.group(2))
            self.avg0_current = float(m.group(3))
            self.avg1_current = float(m.group(4))
            self.cpu0_history.append(self.cpu0_current)
            self.cpu1_history.append(self.cpu1_current)
            self.avg0_history.append(self.avg0_current)
            self.avg1_history.append(self.avg1_current)
            self.timestamps.append(elapsed)
            return

        # IDLE_COUNT
        m = re.match(r'IDLE_COUNT,core0=(\d+),core1=(\d+)', content)
        if m:
            self.idle_core0 = int(m.group(1))
            self.idle_core1 = int(m.group(2))
            self.idle_history_c0.append(self.idle_core0)
            self.idle_history_c1.append(self.idle_core1)
            return

        # LOAD_CHANGE
        m = re.match(r'LOAD_CHANGE,level=(\d+),percent=(\d+)', content)
        if m:
            self.load_level = int(m.group(1))
            self.load_percent = int(m.group(2))
            self.load_changes.append((elapsed, self.load_percent))
            return

        # LOAD_INFO
        m = re.match(r'LOAD_INFO,level=(\d+),target_percent=(\d+)', content)
        if m:
            self.load_level = int(m.group(1))
            self.load_percent = int(m.group(2))
            return

        # HEAP
        m = re.match(r'HEAP,free=(\d+),min=(\d+)', content)
        if m:
            self.heap_free = int(m.group(1))
            self.heap_min = int(m.group(2))
            return

        # CALIBRATION
        m = re.match(r'CALIBRATION,max_idle_core0=(\d+),max_idle_core1=(\d+)', content)
        if m:
            self.max_idle_core0 = int(m.group(1))
            self.max_idle_core1 = int(m.group(2))
            return


class IdleHookVisualizer:
    """Visualisasi real-time CPU usage"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = plt.figure(figsize=(16, 10))
        self.fig.suptitle('ESP32 FreeRTOS - Idle Hook CPU Usage Monitor',
                          fontsize=14, fontweight='bold')
        gs = GridSpec(3, 3, figure=self.fig, hspace=0.4, wspace=0.35)

        # CPU usage over time (line chart)
        self.ax_cpu = self.fig.add_subplot(gs[0, :])
        self.ax_cpu.set_title('CPU Usage Over Time')

        # Current CPU gauge (bar chart)
        self.ax_gauge = self.fig.add_subplot(gs[1, 0])
        self.ax_gauge.set_title('CPU Usage Saat Ini')

        # Load vs measured (line chart)
        self.ax_load = self.fig.add_subplot(gs[1, 1])
        self.ax_load.set_title('Target Load vs Actual')

        # Info panel
        self.ax_info = self.fig.add_subplot(gs[1, 2])
        self.ax_info.axis('off')

        # Idle counter history
        self.ax_idle = self.fig.add_subplot(gs[2, :2])
        self.ax_idle.set_title('Idle Counter History')

        # Heap info
        self.ax_heap = self.fig.add_subplot(gs[2, 2])
        self.ax_heap.set_title('Heap Memory')

    def update(self, frame):
        """Update semua plot"""
        p = self.parser

        # 1. CPU usage line chart
        self.ax_cpu.clear()
        self.ax_cpu.set_title('CPU Usage Over Time (%)')
        self.ax_cpu.set_xlabel('Waktu (s)')
        self.ax_cpu.set_ylabel('CPU Usage (%)')
        self.ax_cpu.set_ylim(0, 105)
        if p.timestamps and p.cpu0_history:
            ts = list(p.timestamps)
            self.ax_cpu.plot(ts[:len(p.cpu0_history)], list(p.cpu0_history),
                           'r-', label='Core 0', alpha=0.8, linewidth=1.5)
            self.ax_cpu.plot(ts[:len(p.cpu1_history)], list(p.cpu1_history),
                           'b-', label='Core 1', alpha=0.8, linewidth=1.5)
            if p.avg0_history:
                self.ax_cpu.plot(ts[:len(p.avg0_history)], list(p.avg0_history),
                               'r--', label='Avg Core 0', alpha=0.4)
                self.ax_cpu.plot(ts[:len(p.avg1_history)], list(p.avg1_history),
                               'b--', label='Avg Core 1', alpha=0.4)
            # Garis load change
            for t, pct in p.load_changes:
                self.ax_cpu.axvline(x=t, color='green', linestyle=':', alpha=0.5)
        self.ax_cpu.legend(loc='upper right', fontsize=8)
        self.ax_cpu.grid(True, alpha=0.3)

        # 2. Current CPU gauge
        self.ax_gauge.clear()
        self.ax_gauge.set_title('CPU Usage Saat Ini (%)')
        cores = ['Core 0', 'Core 1', 'Total']
        total = (p.cpu0_current + p.cpu1_current) / 2
        vals = [p.cpu0_current, p.cpu1_current, total]
        colors = ['#ff6b6b' if v > 80 else '#ffd93d' if v > 50 else '#4ecdc4' for v in vals]
        bars = self.ax_gauge.barh(cores, vals, color=colors)
        self.ax_gauge.set_xlim(0, 105)
        for bar, val in zip(bars, vals):
            self.ax_gauge.text(val + 1, bar.get_y() + bar.get_height()/2.,
                              f'{val:.1f}%', ha='left', va='center', fontsize=9)

        # 3. Target vs Actual
        self.ax_load.clear()
        self.ax_load.set_title('Target Load vs Actual CPU')
        if p.timestamps and p.cpu1_history:
            ts = list(p.timestamps)
            self.ax_load.plot(ts[:len(p.cpu1_history)], list(p.cpu1_history),
                            'b-', label='Actual (Core 1)', alpha=0.8)
            self.ax_load.axhline(y=p.load_percent, color='r', linestyle='--',
                               label=f'Target ({p.load_percent}%)')
        self.ax_load.set_ylim(0, 105)
        self.ax_load.set_xlabel('Waktu (s)')
        self.ax_load.set_ylabel('%')
        self.ax_load.legend(fontsize=8)
        self.ax_load.grid(True, alpha=0.3)

        # 4. Info panel
        self.ax_info.clear()
        self.ax_info.axis('off')
        info = (
            f"═══ STATUS ═══\n"
            f"Load Level: {p.load_level}\n"
            f"Load Target: {p.load_percent}%\n"
            f"\n═══ CALIBRATION ═══\n"
            f"Max Idle C0: {p.max_idle_core0:,}\n"
            f"Max Idle C1: {p.max_idle_core1:,}\n"
            f"\n═══ HEAP ═══\n"
            f"Free: {p.heap_free:,} B\n"
            f"Min:  {p.heap_min:,} B\n"
            f"\n═══ DATA ═══\n"
            f"Parsed: {p.data_lines}\n"
            f"Time: {time.time()-p.start_time:.0f}s"
        )
        self.ax_info.text(0.05, 0.95, info, transform=self.ax_info.transAxes,
                         fontsize=9, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.8))

        # 5. Idle counter
        self.ax_idle.clear()
        self.ax_idle.set_title('Idle Counter (kumulatif)')
        self.ax_idle.set_xlabel('Sampel')
        self.ax_idle.set_ylabel('Idle Count')
        if p.idle_history_c0:
            self.ax_idle.plot(list(p.idle_history_c0), 'r-', label='Core 0', alpha=0.7)
        if p.idle_history_c1:
            self.ax_idle.plot(list(p.idle_history_c1), 'b-', label='Core 1', alpha=0.7)
        self.ax_idle.legend(fontsize=8)
        self.ax_idle.grid(True, alpha=0.3)

        # 6. Heap bar
        self.ax_heap.clear()
        self.ax_heap.set_title('Heap Memory (bytes)')
        cats = ['Free', 'Min Ever']
        vals = [p.heap_free, p.heap_min]
        colors = ['#4ecdc4', '#ffd93d']
        bars = self.ax_heap.bar(cats, vals, color=colors)
        for bar, val in zip(bars, vals):
            self.ax_heap.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                             f'{val:,}', ha='center', va='bottom', fontsize=8)

        return []

    def run(self):
        ani = animation.FuncAnimation(self.fig, self.update, interval=1000, blit=False)
        plt.tight_layout()
        plt.show()


def read_serial(port, baudrate, parser):
    """Baca data dari serial port"""
    if not HAS_SERIAL:
        print("ERROR: pyserial tidak terinstall. pip install pyserial")
        sys.exit(1)
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Terhubung ke {port} @ {baudrate} baud")
        while True:
            line = ser.readline().decode('utf-8', errors='ignore')
            if line:
                parser.parse_line(line)
                if '[DATA]' in line:
                    print(f"  {line.strip()}")
    except serial.SerialException as e:
        print(f"Error serial: {e}")
    except KeyboardInterrupt:
        print("\nDihentikan.")


def read_file(filepath, parser):
    """Baca data dari file log"""
    try:
        with open(filepath, 'r') as f:
            for line in f:
                parser.parse_line(line)
        print(f"Selesai membaca {parser.data_lines} data dari {filepath}")
    except FileNotFoundError:
        print(f"File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description='ESP32 Idle Hook CPU Usage Visualizer')
    ap.add_argument('--port', '-p', help='Serial port')
    ap.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate')
    ap.add_argument('--file', '-f', help='Baca dari file log')
    ap.add_argument('--no-gui', action='store_true', help='Tanpa GUI')
    args = ap.parse_args()

    data_parser = IdleHookParser()

    if args.file:
        read_file(args.file, data_parser)
        if not args.no_gui and HAS_MATPLOTLIB:
            viz = IdleHookVisualizer(data_parser)
            viz.update(0)
            plt.tight_layout()
            plt.show()
        else:
            print(f"\n=== Ringkasan CPU Usage ===")
            print(f"Core 0: {data_parser.cpu0_current:.1f}%")
            print(f"Core 1: {data_parser.cpu1_current:.1f}%")
            print(f"Load target: {data_parser.load_percent}%")
    elif args.port:
        if not args.no_gui and HAS_MATPLOTLIB:
            thread = threading.Thread(target=read_serial,
                                     args=(args.port, args.baud, data_parser),
                                     daemon=True)
            thread.start()
            viz = IdleHookVisualizer(data_parser)
            viz.run()
        else:
            read_serial(args.port, args.baud, data_parser)
    else:
        print("Gunakan --port atau --file")
        print("  python debug_idle_hook.py --port /dev/ttyUSB0")
        print("  python debug_idle_hook.py --file output.log")


if __name__ == '__main__':
    main()

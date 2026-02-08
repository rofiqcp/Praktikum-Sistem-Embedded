#!/usr/bin/env python3
"""
============================================================================
Debug Parser & Visualizer - ESP32 Task Core Affinity
============================================================================
Mem-parse output serial [DATA] dari program ESP32_07_Task_Core_Affinity
dan menampilkan grafik perbandingan performa antar core.

Fitur:
  - Grafik waktu eksekusi per core (line chart)
  - Distribusi core untuk floating task (pie chart)
  - Perbandingan rata-rata waktu eksekusi (bar chart)
  - Stack watermark monitoring
  - Real-time update

Penggunaan:
  python debug_core_affinity.py --port /dev/ttyUSB0
  python debug_core_affinity.py --file output.log
============================================================================
"""

import argparse
import sys
import re
import time
import threading
from collections import defaultdict, deque
from datetime import datetime

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


class CoreAffinityParser:
    """Parser untuk data serial ESP32 Core Affinity"""

    def __init__(self, max_samples=200):
        self.max_samples = max_samples
        # Data waktu eksekusi per task
        self.core0_times = deque(maxlen=max_samples)
        self.core1_times = deque(maxlen=max_samples)
        self.float_times = deque(maxlen=max_samples)
        # Distribusi core floating task
        self.float_core0_count = 0
        self.float_core1_count = 0
        # Statistik rata-rata
        self.avg_core0 = 0
        self.avg_core1 = 0
        self.avg_float = 0
        # Stack watermark
        self.watermarks = {}
        # Heap info
        self.heap_free = 0
        self.heap_min = 0
        # Workload info
        self.workload_level = "MEDIUM"
        self.iterations = 0
        # Perbandingan
        self.compare_ratio = 0
        # Timestamps
        self.timestamps = deque(maxlen=max_samples)
        self.start_time = time.time()
        # Counter untuk statistik
        self.total_lines = 0
        self.data_lines = 0

    def parse_line(self, line):
        """Parse satu baris output serial"""
        self.total_lines += 1
        line = line.strip()

        if not line.startswith("[DATA]"):
            return

        self.data_lines += 1
        content = line[6:]  # Hapus prefix [DATA]
        elapsed = time.time() - self.start_time

        # Parse CORE0_TASK
        m = re.match(r'CORE0_TASK,core=(\d+),time_us=(\d+),count=(\d+),iter=(\d+)', content)
        if m:
            t_us = int(m.group(2))
            self.core0_times.append(t_us)
            self.timestamps.append(elapsed)
            return

        # Parse CORE1_TASK
        m = re.match(r'CORE1_TASK,core=(\d+),time_us=(\d+),count=(\d+),iter=(\d+)', content)
        if m:
            t_us = int(m.group(2))
            self.core1_times.append(t_us)
            return

        # Parse FLOAT_TASK
        m = re.match(r'FLOAT_TASK,core=(\d+),time_us=(\d+),count=(\d+),c0=(\d+),c1=(\d+)', content)
        if m:
            t_us = int(m.group(2))
            self.float_times.append(t_us)
            self.float_core0_count = int(m.group(4))
            self.float_core1_count = int(m.group(5))
            return

        # Parse STATS
        m = re.match(r'STATS,(.+),pinned=(-?\d+),avg_us=(\d+),min_us=(\d+),max_us=(\d+)', content)
        if m:
            name = m.group(1)
            avg = int(m.group(3))
            if 'Core0' in name:
                self.avg_core0 = avg
            elif 'Core1' in name:
                self.avg_core1 = avg
            elif 'Float' in name:
                self.avg_float = avg
            return

        # Parse STACK
        m = re.match(r'STACK,task=(\w+),watermark=(\d+)', content)
        if m:
            self.watermarks[m.group(1)] = int(m.group(2))
            return

        # Parse SYSTEM
        m = re.match(r'SYSTEM,heap_free=(\d+),heap_min=(\d+)', content)
        if m:
            self.heap_free = int(m.group(1))
            self.heap_min = int(m.group(2))
            return

        # Parse COMPARE
        m = re.match(r'COMPARE,avg_core0_us=(\d+),avg_core1_us=(\d+),ratio=([\d.]+)', content)
        if m:
            self.compare_ratio = float(m.group(3))
            return

        # Parse WORKLOAD
        m = re.match(r'WORKLOAD,level=(\w+),iterations=(\d+)', content)
        if m:
            self.workload_level = m.group(1)
            self.iterations = int(m.group(2))
            return


class CoreAffinityVisualizer:
    """Visualisasi real-time data core affinity"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = plt.figure(figsize=(16, 10))
        self.fig.suptitle('ESP32 FreeRTOS - Task Core Affinity Monitor',
                          fontsize=14, fontweight='bold')
        gs = GridSpec(3, 3, figure=self.fig, hspace=0.4, wspace=0.3)

        # Subplot 1: Waktu eksekusi per task (line chart)
        self.ax_time = self.fig.add_subplot(gs[0, :2])
        self.ax_time.set_title('Waktu Eksekusi per Task')
        self.ax_time.set_xlabel('Sampel')
        self.ax_time.set_ylabel('Waktu (μs)')

        # Subplot 2: Distribusi core floating task (pie)
        self.ax_pie = self.fig.add_subplot(gs[0, 2])
        self.ax_pie.set_title('Float Task Core Distribution')

        # Subplot 3: Perbandingan rata-rata (bar)
        self.ax_bar = self.fig.add_subplot(gs[1, 0])
        self.ax_bar.set_title('Rata-rata Waktu Eksekusi')

        # Subplot 4: Stack watermark (bar)
        self.ax_stack = self.fig.add_subplot(gs[1, 1])
        self.ax_stack.set_title('Stack Watermark (bytes)')

        # Subplot 5: Info teks
        self.ax_info = self.fig.add_subplot(gs[1, 2])
        self.ax_info.axis('off')

        # Subplot 6: Histogram waktu eksekusi
        self.ax_hist = self.fig.add_subplot(gs[2, :])
        self.ax_hist.set_title('Distribusi Waktu Eksekusi')

    def update(self, frame):
        """Update semua plot"""
        p = self.parser

        # 1. Line chart waktu eksekusi
        self.ax_time.clear()
        self.ax_time.set_title('Waktu Eksekusi per Task')
        self.ax_time.set_xlabel('Sampel')
        self.ax_time.set_ylabel('Waktu (μs)')
        if p.core0_times:
            self.ax_time.plot(list(p.core0_times), 'r-', label='Core 0 (pinned)', alpha=0.7)
        if p.core1_times:
            self.ax_time.plot(list(p.core1_times), 'b-', label='Core 1 (pinned)', alpha=0.7)
        if p.float_times:
            self.ax_time.plot(list(p.float_times), 'g-', label='Float (any)', alpha=0.7)
        self.ax_time.legend(loc='upper right', fontsize=8)
        self.ax_time.grid(True, alpha=0.3)

        # 2. Pie chart distribusi core
        self.ax_pie.clear()
        self.ax_pie.set_title('Float Task Core Distribution')
        if p.float_core0_count + p.float_core1_count > 0:
            sizes = [p.float_core0_count, p.float_core1_count]
            labels = [f'Core 0\n({p.float_core0_count})', f'Core 1\n({p.float_core1_count})']
            colors = ['#ff6b6b', '#4ecdc4']
            self.ax_pie.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%',
                           startangle=90, textprops={'fontsize': 8})

        # 3. Bar chart rata-rata
        self.ax_bar.clear()
        self.ax_bar.set_title('Rata-rata Waktu Eksekusi (μs)')
        tasks = ['Core 0', 'Core 1', 'Float']
        avgs = [p.avg_core0, p.avg_core1, p.avg_float]
        colors = ['#ff6b6b', '#4ecdc4', '#ffd93d']
        bars = self.ax_bar.bar(tasks, avgs, color=colors)
        for bar, val in zip(bars, avgs):
            if val > 0:
                self.ax_bar.text(bar.get_x() + bar.get_width()/2., bar.get_height() + 50,
                                f'{val}', ha='center', va='bottom', fontsize=8)

        # 4. Stack watermark
        self.ax_stack.clear()
        self.ax_stack.set_title('Stack Watermark (bytes)')
        if p.watermarks:
            names = list(p.watermarks.keys())
            vals = list(p.watermarks.values())
            bars = self.ax_stack.barh(names, vals, color='#6c5ce7')
            for bar, val in zip(bars, vals):
                self.ax_stack.text(val + 10, bar.get_y() + bar.get_height()/2.,
                                 f'{val}', ha='left', va='center', fontsize=8)

        # 5. Info teks
        self.ax_info.clear()
        self.ax_info.axis('off')
        info_text = (
            f"Workload: {p.workload_level}\n"
            f"Iterations: {p.iterations}\n"
            f"Heap Free: {p.heap_free:,} B\n"
            f"Heap Min: {p.heap_min:,} B\n"
            f"Core1/Core0: {p.compare_ratio:.1f}%\n"
            f"Lines: {p.data_lines}/{p.total_lines}\n"
            f"Elapsed: {time.time()-p.start_time:.0f}s"
        )
        self.ax_info.text(0.1, 0.9, info_text, transform=self.ax_info.transAxes,
                         fontsize=10, verticalalignment='top',
                         fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

        # 6. Histogram
        self.ax_hist.clear()
        self.ax_hist.set_title('Distribusi Waktu Eksekusi (μs)')
        self.ax_hist.set_xlabel('Waktu (μs)')
        self.ax_hist.set_ylabel('Frekuensi')
        if p.core0_times:
            self.ax_hist.hist(list(p.core0_times), bins=30, alpha=0.5,
                             label='Core 0', color='red')
        if p.core1_times:
            self.ax_hist.hist(list(p.core1_times), bins=30, alpha=0.5,
                             label='Core 1', color='blue')
        if p.float_times:
            self.ax_hist.hist(list(p.float_times), bins=30, alpha=0.5,
                             label='Float', color='green')
        self.ax_hist.legend(fontsize=8)
        self.ax_hist.grid(True, alpha=0.3)

        return []

    def run(self):
        """Mulai animasi"""
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
    parser_args = argparse.ArgumentParser(
        description='ESP32 Core Affinity Debug Parser & Visualizer')
    parser_args.add_argument('--port', '-p', help='Serial port (contoh: /dev/ttyUSB0)')
    parser_args.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate')
    parser_args.add_argument('--file', '-f', help='Baca dari file log')
    parser_args.add_argument('--no-gui', action='store_true', help='Tanpa GUI (text only)')
    args = parser_args.parse_args()

    data_parser = CoreAffinityParser()

    if args.file:
        read_file(args.file, data_parser)
        if not args.no_gui and HAS_MATPLOTLIB:
            viz = CoreAffinityVisualizer(data_parser)
            viz.update(0)
            plt.tight_layout()
            plt.show()
        else:
            print(f"\n=== Ringkasan ===")
            print(f"Core 0 avg: {data_parser.avg_core0} μs")
            print(f"Core 1 avg: {data_parser.avg_core1} μs")
            print(f"Float C0/C1: {data_parser.float_core0_count}/{data_parser.float_core1_count}")
    elif args.port:
        if not args.no_gui and HAS_MATPLOTLIB:
            thread = threading.Thread(target=read_serial,
                                     args=(args.port, args.baud, data_parser),
                                     daemon=True)
            thread.start()
            viz = CoreAffinityVisualizer(data_parser)
            viz.run()
        else:
            read_serial(args.port, args.baud, data_parser)
    else:
        print("Gunakan --port atau --file. Contoh:")
        print("  python debug_core_affinity.py --port /dev/ttyUSB0")
        print("  python debug_core_affinity.py --file output.log")


if __name__ == '__main__':
    main()

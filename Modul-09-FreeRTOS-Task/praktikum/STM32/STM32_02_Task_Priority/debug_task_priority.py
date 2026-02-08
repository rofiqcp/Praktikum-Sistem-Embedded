#!/usr/bin/env python3
"""
============================================================================
Debug & Visualisasi: STM32_02_Task_Priority
============================================================================

Parser serial output dan visualisasi data dari program FreeRTOS Task Priority.
Menampilkan grafik real-time:
  1. Execution time per task per fase (grouped bar)
  2. Priority changes timeline
  3. Execution order visualization
  4. Preemption analysis

Format data yang di-parse:
  [DATA]EXEC,<nama>,<prioritas>,<elapsed_us>,<iteration>,<tick>
  [DATA]PHASE,<nomor>,<deskripsi>,<tick>
  [DATA]PRIORITY,<nama>,<old_prio>,<new_prio>,<tick>
  [DATA]ORDER,<urutan>,<tick>
  [DATA]HEAP,<free>,<tick>

Usage:
  python debug_task_priority.py --port /dev/ttyUSB0
  python debug_task_priority.py --file output.log
  python debug_task_priority.py --demo

============================================================================
"""

import sys
import argparse
import time
import re
from collections import defaultdict
from datetime import datetime

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.gridspec import GridSpec
    import matplotlib.patches as mpatches
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak ditemukan. Install: pip install matplotlib")

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial tidak ditemukan. Install: pip install pyserial")


class TaskPriorityParser:
    """Parser untuk data serial dari program Task Priority"""

    def __init__(self):
        # Data eksekusi per task
        self.exec_data = defaultdict(lambda: {
            'priorities': [],
            'elapsed_us': [],
            'iterations': [],
            'ticks': []
        })

        # Data perubahan fase
        self.phase_data = {
            'numbers': [],
            'descriptions': [],
            'ticks': []
        }

        # Data perubahan prioritas
        self.priority_changes = {
            'names': [],
            'old_prios': [],
            'new_prios': [],
            'ticks': []
        }

        # Data urutan eksekusi
        self.exec_orders = {
            'orders': [],
            'ticks': []
        }

        # Heap data
        self.heap_data = {
            'free': [],
            'ticks': []
        }

        self.raw_lines = []
        self.current_phase = 1

        # Regex patterns
        self.re_exec = re.compile(
            r'\[DATA\]EXEC,(\w+),(\d+),(\d+),(\d+),(\d+)'
        )
        self.re_phase = re.compile(
            r'\[DATA\]PHASE,(\d+),(\w+),(\d+)'
        )
        self.re_priority = re.compile(
            r'\[DATA\]PRIORITY,(\w+),(\d+),(\d+),(\d+)'
        )
        self.re_order = re.compile(
            r'\[DATA\]ORDER,([LMH]*),(\d+)'
        )
        self.re_heap = re.compile(
            r'\[DATA\]HEAP,(\d+),(\d+)'
        )

    def parse_line(self, line):
        """Parse satu baris output serial"""
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # Parse EXEC data
        m = self.re_exec.match(line)
        if m:
            name = m.group(1)
            prio = int(m.group(2))
            elapsed = int(m.group(3))
            iteration = int(m.group(4))
            tick = int(m.group(5))

            self.exec_data[name]['priorities'].append(prio)
            self.exec_data[name]['elapsed_us'].append(elapsed)
            self.exec_data[name]['iterations'].append(iteration)
            self.exec_data[name]['ticks'].append(tick)
            return

        # Parse PHASE data
        m = self.re_phase.match(line)
        if m:
            num = int(m.group(1))
            desc = m.group(2)
            tick = int(m.group(3))

            self.phase_data['numbers'].append(num)
            self.phase_data['descriptions'].append(desc)
            self.phase_data['ticks'].append(tick)
            self.current_phase = num
            return

        # Parse PRIORITY change
        m = self.re_priority.match(line)
        if m:
            name = m.group(1)
            old_p = int(m.group(2))
            new_p = int(m.group(3))
            tick = int(m.group(4))

            self.priority_changes['names'].append(name)
            self.priority_changes['old_prios'].append(old_p)
            self.priority_changes['new_prios'].append(new_p)
            self.priority_changes['ticks'].append(tick)
            return

        # Parse ORDER
        m = self.re_order.match(line)
        if m:
            order = m.group(1)
            tick = int(m.group(2))

            self.exec_orders['orders'].append(order)
            self.exec_orders['ticks'].append(tick)
            return

        # Parse HEAP
        m = self.re_heap.match(line)
        if m:
            free = int(m.group(1))
            tick = int(m.group(2))

            self.heap_data['free'].append(free)
            self.heap_data['ticks'].append(tick)
            return

    def get_summary(self):
        """Ringkasan data"""
        summary = []
        summary.append("=" * 65)
        summary.append("  RINGKASAN DATA - Task Priority & Preemption")
        summary.append("=" * 65)

        for name, data in self.exec_data.items():
            summary.append(f"\n  Task: {name}")
            if data['elapsed_us']:
                avg_us = sum(data['elapsed_us']) / len(data['elapsed_us'])
                min_us = min(data['elapsed_us'])
                max_us = max(data['elapsed_us'])
                summary.append(f"    Exec Count  : {len(data['elapsed_us'])}")
                summary.append(f"    Avg Time    : {avg_us:.0f} us")
                summary.append(f"    Min Time    : {min_us} us")
                summary.append(f"    Max Time    : {max_us} us")
                if data['priorities']:
                    summary.append(f"    Last Prio   : {data['priorities'][-1]}")

        summary.append(f"\n  Fase:")
        summary.append(f"    Total Changes : {len(self.phase_data['numbers'])}")
        summary.append(f"    Current Phase : {self.current_phase}")

        summary.append(f"\n  Priority Changes: {len(self.priority_changes['names'])}")
        for i, name in enumerate(self.priority_changes['names']):
            old = self.priority_changes['old_prios'][i]
            new = self.priority_changes['new_prios'][i]
            summary.append(f"    {name}: {old} → {new}")

        if self.exec_orders['orders']:
            summary.append(f"\n  Last Exec Order: {self.exec_orders['orders'][-1][:40]}...")

        summary.append("=" * 65)
        return "\n".join(summary)


class TaskPriorityVisualizer:
    """Visualisasi data Task Priority"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = None

        self.task_colors = {
            'Task_Low': '#4CAF50',
            'Task_Med': '#FF9800',
            'Task_High': '#F44336',
        }

    def setup_plot(self):
        """Setup figure"""
        self.fig = plt.figure(figsize=(16, 11))
        self.fig.suptitle('STM32 FreeRTOS - Task Priority & Preemption Analysis',
                          fontsize=14, fontweight='bold')

        gs = GridSpec(2, 2, figure=self.fig, hspace=0.35, wspace=0.3)

        self.ax_exec = self.fig.add_subplot(gs[0, 0])
        self.ax_prio = self.fig.add_subplot(gs[0, 1])
        self.ax_order = self.fig.add_subplot(gs[1, 0])
        self.ax_timeline = self.fig.add_subplot(gs[1, 1])

    def update_plot(self, frame=None):
        """Update semua subplot"""
        p = self.parser

        # --- Execution Time per Task ---
        self.ax_exec.clear()
        self.ax_exec.set_title('Waktu Eksekusi per Task (us)', fontsize=11)

        task_names = list(p.exec_data.keys())
        if task_names:
            positions = range(len(task_names))
            avg_times = []
            min_times = []
            max_times = []
            colors = []

            for name in task_names:
                data = p.exec_data[name]
                if data['elapsed_us']:
                    avg_times.append(sum(data['elapsed_us']) / len(data['elapsed_us']))
                    min_times.append(min(data['elapsed_us']))
                    max_times.append(max(data['elapsed_us']))
                else:
                    avg_times.append(0)
                    min_times.append(0)
                    max_times.append(0)
                colors.append(self.task_colors.get(name, '#9E9E9E'))

            short_names = [n.replace('Task_', '') for n in task_names]
            bars = self.ax_exec.bar(short_names, avg_times, color=colors,
                                    edgecolor='black', alpha=0.8)

            # Error bars (min-max range)
            err_low = [a - m for a, m in zip(avg_times, min_times)]
            err_high = [m - a for a, m in zip(avg_times, max_times)]
            self.ax_exec.errorbar(short_names, avg_times,
                                   yerr=[err_low, err_high],
                                   fmt='none', color='black', capsize=5)

            for bar, avg in zip(bars, avg_times):
                self.ax_exec.text(bar.get_x() + bar.get_width()/2.,
                                   bar.get_height(),
                                   f'{avg:.0f}', ha='center', va='bottom',
                                   fontsize=9)

            self.ax_exec.set_ylabel('Waktu (us)')
            self.ax_exec.grid(axis='y', alpha=0.3)

        # --- Priority Timeline ---
        self.ax_prio.clear()
        self.ax_prio.set_title('Prioritas Task Sepanjang Waktu', fontsize=11)

        for name, data in p.exec_data.items():
            if data['ticks'] and data['priorities']:
                ticks_sec = [t / 1000.0 for t in data['ticks']]
                color = self.task_colors.get(name, '#9E9E9E')
                self.ax_prio.plot(ticks_sec, data['priorities'],
                                   '-o', markersize=3, color=color,
                                   label=name.replace('Task_', ''),
                                   linewidth=2)

        # Phase boundaries
        for i, tick in enumerate(p.phase_data['ticks']):
            t_sec = tick / 1000.0
            self.ax_prio.axvline(x=t_sec, color='gray', linestyle='--', alpha=0.5)
            if i < len(p.phase_data['descriptions']):
                self.ax_prio.text(t_sec, 6.2,
                                   f"F{p.phase_data['numbers'][i]}",
                                   ha='center', fontsize=8,
                                   bbox=dict(boxstyle='round', facecolor='yellow',
                                            alpha=0.5))

        self.ax_prio.set_xlabel('Waktu (detik)')
        self.ax_prio.set_ylabel('Prioritas')
        self.ax_prio.set_ylim(0, 7)
        self.ax_prio.legend(fontsize=8)
        self.ax_prio.grid(True, alpha=0.3)

        # --- Execution Order ---
        self.ax_order.clear()
        self.ax_order.set_title('Urutan Eksekusi Task', fontsize=11)

        if p.exec_orders['orders']:
            last_order = p.exec_orders['orders'][-1][:50]

            color_map = {'L': '#4CAF50', 'M': '#FF9800', 'H': '#F44336'}
            x_pos = range(len(last_order))
            y_vals = []
            bar_colors = []

            for c in last_order:
                if c == 'H':
                    y_vals.append(3)
                elif c == 'M':
                    y_vals.append(2)
                else:
                    y_vals.append(1)
                bar_colors.append(color_map.get(c, '#9E9E9E'))

            self.ax_order.bar(x_pos, y_vals, color=bar_colors, edgecolor='none',
                               width=1.0)
            self.ax_order.set_yticks([1, 2, 3])
            self.ax_order.set_yticklabels(['Low', 'Med', 'High'])
            self.ax_order.set_xlabel('Urutan Eksekusi')
            self.ax_order.set_ylabel('Task')

            # Legend
            patches = [
                mpatches.Patch(color='#F44336', label='High'),
                mpatches.Patch(color='#FF9800', label='Med'),
                mpatches.Patch(color='#4CAF50', label='Low'),
            ]
            self.ax_order.legend(handles=patches, fontsize=8, loc='upper right')
            self.ax_order.grid(axis='y', alpha=0.3)

        # --- Exec Time Timeline ---
        self.ax_timeline.clear()
        self.ax_timeline.set_title('Execution Time Timeline', fontsize=11)

        for name, data in p.exec_data.items():
            if data['ticks'] and data['elapsed_us']:
                ticks_sec = [t / 1000.0 for t in data['ticks']]
                color = self.task_colors.get(name, '#9E9E9E')
                self.ax_timeline.plot(ticks_sec, data['elapsed_us'],
                                       '-', markersize=2, color=color,
                                       label=name.replace('Task_', ''),
                                       linewidth=1.2, alpha=0.8)

        # Phase boundaries
        for tick in p.phase_data['ticks']:
            self.ax_timeline.axvline(x=tick / 1000.0, color='gray',
                                      linestyle='--', alpha=0.5)

        self.ax_timeline.set_xlabel('Waktu (detik)')
        self.ax_timeline.set_ylabel('Execution Time (us)')
        self.ax_timeline.legend(fontsize=8)
        self.ax_timeline.grid(True, alpha=0.3)

        self.fig.canvas.draw_idle()

    def show(self):
        """Tampilkan plot"""
        self.setup_plot()
        self.update_plot()
        plt.tight_layout()
        plt.show()


def generate_demo_data(parser):
    """Generate data demo"""
    import random
    random.seed(42)

    print("  Generating demo data for Task Priority...")

    base_work_us = 6944  # ~500K iterations at 72MHz

    # Phase 1: Normal priorities (Low=1, Med=3, High=5)
    parser.parse_line("[DATA]PHASE,1,Normal,0")

    for i in range(20):
        tick = i * 1000

        # High priority finishes fastest (no preemption)
        h_time = base_work_us + random.randint(-50, 50)
        parser.parse_line(f"[DATA]EXEC,Task_High,5,{h_time},{i+1},{tick}")

        # Medium gets preempted by high sometimes
        m_time = base_work_us + random.randint(200, 800)
        parser.parse_line(f"[DATA]EXEC,Task_Med,3,{m_time},{i+1},{tick + 100}")

        # Low gets preempted by both
        l_time = base_work_us + random.randint(500, 2000)
        parser.parse_line(f"[DATA]EXEC,Task_Low,1,{l_time},{i+1},{tick + 200}")

    # Phase 2: Swapped (Low=5, Med=3, High=1)
    parser.parse_line("[DATA]PHASE,2,Swapped,20000")
    parser.parse_line("[DATA]PRIORITY,Task_Low,1,5,20000")
    parser.parse_line("[DATA]PRIORITY,Task_High,5,1,20000")

    for i in range(20):
        tick = 20000 + i * 1000

        # Low now has highest priority, finishes fast
        l_time = base_work_us + random.randint(-50, 50)
        parser.parse_line(f"[DATA]EXEC,Task_Low,5,{l_time},{20+i+1},{tick}")

        m_time = base_work_us + random.randint(200, 800)
        parser.parse_line(f"[DATA]EXEC,Task_Med,3,{m_time},{20+i+1},{tick + 100}")

        # High now slowest
        h_time = base_work_us + random.randint(500, 2000)
        parser.parse_line(f"[DATA]EXEC,Task_High,1,{h_time},{20+i+1},{tick + 200}")

    # Phase 3: Restored
    parser.parse_line("[DATA]PHASE,3,Restored,40000")
    parser.parse_line("[DATA]PRIORITY,Task_Low,5,1,40000")
    parser.parse_line("[DATA]PRIORITY,Task_High,1,5,40000")

    for i in range(10):
        tick = 40000 + i * 1000
        h_time = base_work_us + random.randint(-50, 50)
        parser.parse_line(f"[DATA]EXEC,Task_High,5,{h_time},{40+i+1},{tick}")
        m_time = base_work_us + random.randint(200, 800)
        parser.parse_line(f"[DATA]EXEC,Task_Med,3,{m_time},{40+i+1},{tick + 100}")
        l_time = base_work_us + random.randint(500, 2000)
        parser.parse_line(f"[DATA]EXEC,Task_Low,1,{l_time},{40+i+1},{tick + 200}")

    # Execution orders
    parser.parse_line("[DATA]ORDER,HHMHLMHLMHHMH,15000")
    parser.parse_line("[DATA]ORDER,LLMLHLMLLLML,35000")
    parser.parse_line("[DATA]ORDER,HHMHLMHLMHHML,48000")

    # Heap data
    for i in range(25):
        tick = i * 2000
        free = 5200 - random.randint(0, 40)
        parser.parse_line(f"[DATA]HEAP,{free},{tick}")

    print(f"  Generated {len(parser.raw_lines)} data points")


def read_serial(port, baudrate, parser, visualizer=None):
    """Baca data dari serial port"""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan.")
        return

    print(f"  Menghubungkan ke {port} @ {baudrate} baud...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"  Terhubung! Tekan Ctrl+C untuk berhenti.\n")

        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    print(f"  >> {line}")
                    parser.parse_line(line)

                    if visualizer and len(parser.raw_lines) % 10 == 0:
                        visualizer.update_plot()

    except KeyboardInterrupt:
        print("\n  Dihentikan oleh user.")
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    finally:
        if 'ser' in locals():
            ser.close()


def read_file(filepath, parser):
    """Baca dari file log"""
    print(f"  Membaca file: {filepath}")
    try:
        with open(filepath, 'r') as f:
            for line in f:
                parser.parse_line(line)
        print(f"  Selesai - {len(parser.raw_lines)} baris")
    except FileNotFoundError:
        print(f"[ERROR] File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    """Entry point"""
    arg_parser = argparse.ArgumentParser(
        description='Debug & Visualisasi STM32 FreeRTOS Task Priority',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  %(prog)s --port /dev/ttyUSB0
  %(prog)s --file output.log
  %(prog)s --demo
        """
    )

    arg_parser.add_argument('--port', type=str, help='Serial port')
    arg_parser.add_argument('--baud', type=int, default=115200, help='Baudrate')
    arg_parser.add_argument('--file', type=str, help='File log')
    arg_parser.add_argument('--demo', action='store_true', help='Mode demo')
    arg_parser.add_argument('--no-plot', action='store_true', help='Tanpa grafik')

    args = arg_parser.parse_args()

    print()
    print("=" * 65)
    print("  STM32 FreeRTOS - Task Priority - Debug Tool")
    print(f"  Waktu: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 65)
    print()

    parser = TaskPriorityParser()
    visualizer = None

    if not args.no_plot and HAS_MATPLOTLIB:
        visualizer = TaskPriorityVisualizer(parser)

    if args.demo:
        generate_demo_data(parser)
        print()
        print(parser.get_summary())
        if visualizer:
            visualizer.show()

    elif args.file:
        read_file(args.file, parser)
        print()
        print(parser.get_summary())
        if visualizer:
            visualizer.show()

    elif args.port:
        read_serial(args.port, args.baud, parser, visualizer)
        print()
        print(parser.get_summary())
        if visualizer:
            visualizer.show()

    else:
        print("  Mode demo (default)...")
        print()
        generate_demo_data(parser)
        print()
        print(parser.get_summary())
        if visualizer:
            visualizer.show()


if __name__ == '__main__':
    main()

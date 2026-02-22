#!/usr/bin/env python3
"""
============================================================================
Debug Parser & Visualizer - ESP32 Task Scheduler Info
============================================================================
Mem-parse output serial [DATA] dari program ESP32_11_Task_Scheduler_Info
dan menampilkan tabel task dan runtime stats secara visual.

Fitur:
  - Tabel task list dengan state/priority/stack
  - Pie chart distribusi CPU time
  - Grafik jumlah task over time
  - Stack watermark bar chart
  - Runtime statistics per task
  - Phase timeline

Penggunaan:
  python debug_scheduler_info.py --port /dev/ttyUSB0
  python debug_scheduler_info.py --file output.log
============================================================================
"""

import argparse
import sys
import re
import time
import threading
from collections import deque, defaultdict, OrderedDict

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


class SchedulerInfoParser:
    """Parser untuk data serial ESP32 Scheduler Info"""

    def __init__(self, max_samples=300):
        self.max_samples = max_samples
        # Task list data
        self.task_list = OrderedDict()  # name -> {state, priority, stack, num}
        # Runtime stats
        self.runtime_stats = {}  # name -> {abs_time, pct}
        # System state
        self.sys_state = {}  # name -> full dict
        # Task count history
        self.task_count_history = deque(maxlen=max_samples)
        self.timestamps = deque(maxlen=max_samples)
        # Phase info
        self.current_phase = 0
        self.phase_changes = []
        # Worker counts
        self.worker1_count = 0
        self.worker2_count = 0
        self.dynamic_count = 0
        # Watermarks
        self.watermarks = {}
        # Dynamic task events
        self.dynamic_events = deque(maxlen=50)
        # Heap
        self.heap_free = 0
        self.heap_min = 0
        # Total task count
        self.total_tasks = 0
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

        # TASK_LIST
        m = re.match(r'TASK_LIST,name=(\w+),state=(\w),priority=(\d+),stack=(\d+),num=(\d+)', content)
        if m:
            self.task_list[m.group(1)] = {
                'state': m.group(2),
                'priority': int(m.group(3)),
                'stack': int(m.group(4)),
                'num': int(m.group(5))
            }
            return

        # RUNTIME
        m = re.match(r'RUNTIME,name=(\w+),abs_time=(\d+),pct=(\d+)', content)
        if m:
            self.runtime_stats[m.group(1)] = {
                'abs_time': int(m.group(2)),
                'pct': int(m.group(3))
            }
            return

        # TOTAL_TASKS
        m = re.match(r'TOTAL_TASKS,count=(\d+)', content)
        if m:
            self.total_tasks = int(m.group(1))
            return

        # TASK_COUNT
        m = re.match(r'TASK_COUNT,total=(\d+),phase=(\d+)', content)
        if m:
            self.total_tasks = int(m.group(1))
            self.task_count_history.append(self.total_tasks)
            self.timestamps.append(elapsed)
            return

        # SYS_STATE
        m = re.match(r'SYS_STATE,num=(\d+),name=(\w+),state=(\d+),prio=(\d+),'
                     r'stack=(\d+),runtime=(\d+),base_prio=(\d+)', content)
        if m:
            self.sys_state[m.group(2)] = {
                'num': int(m.group(1)),
                'state': int(m.group(3)),
                'priority': int(m.group(4)),
                'stack': int(m.group(5)),
                'runtime': int(m.group(6)),
                'base_prio': int(m.group(7))
            }
            return

        # PHASE_CHANGE
        m = re.match(r'PHASE_CHANGE,phase=(\d+)', content)
        if m:
            self.current_phase = int(m.group(1))
            self.phase_changes.append((elapsed, self.current_phase))
            return

        # WORKER_COUNTS
        m = re.match(r'WORKER_COUNTS,w1=(\d+),w2=(\d+),dyn=(\d+)', content)
        if m:
            self.worker1_count = int(m.group(1))
            self.worker2_count = int(m.group(2))
            self.dynamic_count = int(m.group(3))
            return

        # WATERMARK
        m = re.match(r'WATERMARK,task=(\w+),hwm=(\d+)', content)
        if m:
            self.watermarks[m.group(1)] = int(m.group(2))
            return

        # DYNAMIC events
        m = re.match(r'DYNAMIC_(CREATE|DELETE),num=(\d+)', content)
        if m:
            self.dynamic_events.append((elapsed, m.group(1), int(m.group(2))))
            return

        # TASK_SUSPEND/RESUME
        m = re.match(r'TASK_(SUSPEND|RESUME),name=(\w+)', content)
        if m:
            self.dynamic_events.append((elapsed, m.group(1), m.group(2)))
            return

        # HEAP
        m = re.match(r'HEAP,free=(\d+),min=(\d+)', content)
        if m:
            self.heap_free = int(m.group(1))
            self.heap_min = int(m.group(2))
            return


class SchedulerInfoVisualizer:
    """Visualisasi scheduler info"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = plt.figure(figsize=(16, 10))
        self.fig.suptitle('ESP32 FreeRTOS - Task Scheduler Info Monitor',
                          fontsize=14, fontweight='bold')
        gs = GridSpec(3, 3, figure=self.fig, hspace=0.45, wspace=0.35)

        self.ax_table = self.fig.add_subplot(gs[0, :2])
        self.ax_table.axis('off')
        self.ax_pie = self.fig.add_subplot(gs[0, 2])
        self.ax_count = self.fig.add_subplot(gs[1, 0])
        self.ax_stack = self.fig.add_subplot(gs[1, 1])
        self.ax_info = self.fig.add_subplot(gs[1, 2])
        self.ax_info.axis('off')
        self.ax_events = self.fig.add_subplot(gs[2, :2])
        self.ax_events.axis('off')
        self.ax_workers = self.fig.add_subplot(gs[2, 2])

    def _state_name(self, code):
        states = {'X': 'Running', 'R': 'Ready', 'B': 'Blocked',
                  'S': 'Suspended', 'D': 'Deleted'}
        return states.get(code, code)

    def update(self, frame):
        p = self.parser

        # 1. Task table
        self.ax_table.clear()
        self.ax_table.axis('off')
        self.ax_table.set_title(f'Task List ({p.total_tasks} tasks, Phase {p.current_phase})',
                               fontsize=11, loc='left')
        if p.task_list:
            col_labels = ['Task', 'State', 'Priority', 'Stack', '#']
            rows = []
            colors = []
            state_colors = {'X': '#ff6b6b', 'R': '#4ecdc4', 'B': '#ffd93d',
                           'S': '#dfe6e9', 'D': '#636e72'}
            for name, info in p.task_list.items():
                rows.append([name, self._state_name(info['state']),
                           str(info['priority']), str(info['stack']),
                           str(info['num'])])
                c = state_colors.get(info['state'], 'white')
                colors.append([c, c, c, c, c])
            if rows:
                table = self.ax_table.table(cellText=rows, colLabels=col_labels,
                                           cellColours=colors, loc='center',
                                           cellLoc='center')
                table.auto_set_font_size(False)
                table.set_fontsize(8)
                table.scale(1, 1.3)

        # 2. CPU time pie
        self.ax_pie.clear()
        self.ax_pie.set_title('CPU Time Distribution')
        if p.runtime_stats:
            non_zero = {k: v['pct'] for k, v in p.runtime_stats.items() if v['pct'] > 0}
            if non_zero:
                self.ax_pie.pie(non_zero.values(), labels=non_zero.keys(),
                               autopct='%1.0f%%', textprops={'fontsize': 7})

        # 3. Task count over time
        self.ax_count.clear()
        self.ax_count.set_title('Task Count Over Time')
        if p.timestamps and p.task_count_history:
            ts = list(p.timestamps)
            counts = list(p.task_count_history)
            self.ax_count.plot(ts[:len(counts)], counts, 'b-o', markersize=3)
            for t, phase in p.phase_changes:
                self.ax_count.axvline(x=t, color='orange', linestyle=':', alpha=0.5)
        self.ax_count.set_xlabel('Waktu (s)')
        self.ax_count.set_ylabel('Jumlah Task')
        self.ax_count.grid(True, alpha=0.3)

        # 4. Stack watermarks
        self.ax_stack.clear()
        self.ax_stack.set_title('Stack High Water Mark')
        if p.watermarks:
            names = list(p.watermarks.keys())
            vals = list(p.watermarks.values())
            colors = ['#ff6b6b' if v < 200 else '#ffd93d' if v < 500 else '#4ecdc4' for v in vals]
            bars = self.ax_stack.barh(names, vals, color=colors)
            for bar, val in zip(bars, vals):
                self.ax_stack.text(val + 5, bar.get_y() + bar.get_height()/2.,
                                 str(val), ha='left', va='center', fontsize=8)

        # 5. Info panel
        self.ax_info.clear()
        self.ax_info.axis('off')
        phase_names = {0: 'All Active', 1: 'Suspend Task',
                      2: 'Dynamic Create', 3: 'Cleanup'}
        info = (
            f"═══ PHASE ═══\n"
            f"Current: {p.current_phase}\n"
            f"Desc: {phase_names.get(p.current_phase, '?')}\n"
            f"\n═══ WORKERS ═══\n"
            f"Worker1: {p.worker1_count:,}\n"
            f"Worker2: {p.worker2_count:,}\n"
            f"Dynamic: {p.dynamic_count:,}\n"
            f"\n═══ HEAP ═══\n"
            f"Free: {p.heap_free:,} B\n"
            f"Min:  {p.heap_min:,} B\n"
            f"\nTasks: {p.total_tasks}"
        )
        self.ax_info.text(0.05, 0.95, info, transform=self.ax_info.transAxes,
                         fontsize=9, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.8))

        # 6. Events log
        self.ax_events.clear()
        self.ax_events.axis('off')
        self.ax_events.set_title('Dynamic Task Events', fontsize=10, loc='left')
        evt_text = "Time(s)  Event      Detail\n" + "─" * 40 + "\n"
        for t, evt, detail in list(p.dynamic_events)[-10:]:
            evt_text += f"{t:7.1f}s  {evt:10s} {detail}\n"
        if not p.dynamic_events:
            evt_text += "  (belum ada event)\n"
        self.ax_events.text(0.02, 0.95, evt_text, transform=self.ax_events.transAxes,
                           fontsize=8, verticalalignment='top', fontfamily='monospace',
                           bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

        # 7. Worker counts bar
        self.ax_workers.clear()
        self.ax_workers.set_title('Worker Execution Count')
        cats = ['Worker1', 'Worker2', 'Dynamic']
        vals = [p.worker1_count, p.worker2_count, p.dynamic_count]
        colors = ['#4ecdc4', '#6c5ce7', '#fd79a8']
        bars = self.ax_workers.bar(cats, vals, color=colors)
        for bar, val in zip(bars, vals):
            self.ax_workers.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                               f'{val:,}', ha='center', va='bottom', fontsize=8)
        self.ax_workers.tick_params(axis='x', rotation=15)

        return []

    def run(self):
        ani = animation.FuncAnimation(self.fig, self.update, interval=1500, blit=False)
        plt.tight_layout()
        plt.show()


def read_serial(port, baudrate, parser):
    if not HAS_SERIAL:
        print("ERROR: pip install pyserial")
        sys.exit(1)
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Terhubung ke {port} @ {baudrate}")
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
    try:
        with open(filepath, 'r') as f:
            for line in f:
                parser.parse_line(line)
        print(f"Selesai: {parser.data_lines} data lines")
    except FileNotFoundError:
        print(f"File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description='ESP32 Scheduler Info Visualizer')
    ap.add_argument('--port', '-p', help='Serial port')
    ap.add_argument('--baud', '-b', type=int, default=115200)
    ap.add_argument('--file', '-f', help='File log')
    ap.add_argument('--no-gui', action='store_true')
    args = ap.parse_args()

    data_parser = SchedulerInfoParser()

    if args.file:
        read_file(args.file, data_parser)
        if not args.no_gui and HAS_MATPLOTLIB:
            viz = SchedulerInfoVisualizer(data_parser)
            viz.update(0)
            plt.tight_layout()
            plt.show()
        else:
            print(f"\nTotal tasks: {data_parser.total_tasks}")
            for name, info in data_parser.task_list.items():
                print(f"  {name}: state={info['state']}, prio={info['priority']}")
    elif args.port:
        if not args.no_gui and HAS_MATPLOTLIB:
            thread = threading.Thread(target=read_serial,
                                     args=(args.port, args.baud, data_parser),
                                     daemon=True)
            thread.start()
            viz = SchedulerInfoVisualizer(data_parser)
            viz.run()
        else:
            read_serial(args.port, args.baud, data_parser)
    else:
        print("Gunakan --port atau --file")
        print("  python debug_scheduler_info.py --port /dev/ttyUSB0")


if __name__ == '__main__':
    main()

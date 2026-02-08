#!/usr/bin/env python3
"""
============================================================================
Debug Parser & Visualizer - ESP32 Task Watchdog Timer (TWDT)
============================================================================
Mem-parse output serial [DATA] dari program ESP32_09_Task_Watchdog
dan menampilkan timeline event watchdog secara real-time.

Fitur:
  - Timeline event feed/timeout/recovery
  - Grafik feed count per task
  - Status skenario dan violasi
  - Bar chart perbandingan feed tiap task
  - Event log terbaru

Penggunaan:
  python debug_watchdog.py --port /dev/ttyUSB0
  python debug_watchdog.py --file output.log
============================================================================
"""

import argparse
import sys
import re
import time
import threading
from collections import deque, defaultdict

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


class WatchdogParser:
    """Parser untuk data serial ESP32 Watchdog"""

    def __init__(self, max_samples=300):
        self.max_samples = max_samples
        # Feed counts per task
        self.feed_counts = defaultdict(int)
        self.feed_history = defaultdict(lambda: deque(maxlen=max_samples))
        self.timestamps = deque(maxlen=max_samples)
        # Violations
        self.violations = 0
        self.violation_times = []
        # Scenarios
        self.current_scenario = 0
        self.scenario_changes = []
        # Events
        self.events = deque(maxlen=100)
        # Task subscriptions
        self.subscriptions = {}
        # Monitor data
        self.monitor_data = {}
        # Config
        self.wdt_timeout = 5
        # Heap
        self.heap_free = 0
        self.heap_min = 0
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

        # WDT_FEED
        m = re.match(r'WDT_FEED,task=(\w+),count=(\d+)', content)
        if m:
            task = m.group(1)
            count = int(m.group(2))
            self.feed_counts[task] = count
            self.feed_history[task].append(count)
            self.timestamps.append(elapsed)
            self.events.append((elapsed, task, 'FEED', count))
            return

        # WDT_ADD
        m = re.match(r'WDT_ADD,task=(\w+),status=(\w+)', content)
        if m:
            self.subscriptions[m.group(1)] = m.group(2)
            self.events.append((elapsed, m.group(1), 'ADD', 0))
            return

        # WDT_DELETE
        m = re.match(r'WDT_DELETE,task=(\w+),status=(\w+)', content)
        if m:
            self.subscriptions[m.group(1)] = m.group(2)
            self.events.append((elapsed, m.group(1), 'DELETE', 0))
            return

        # WDT_BLOCK
        m = re.match(r'WDT_BLOCK,task=(\w+),blocking=(\w+)', content)
        if m:
            self.events.append((elapsed, m.group(1), 'BLOCK', 0))
            return

        # WDT_TIMEOUT
        m = re.match(r'WDT_TIMEOUT,task=(\w+),violations=(\d+)', content)
        if m:
            self.violations = int(m.group(2))
            self.violation_times.append(elapsed)
            self.events.append((elapsed, m.group(1), 'TIMEOUT', self.violations))
            return

        # SCENARIO
        m = re.match(r'SCENARIO,num=(\d+),desc=(.+)', content)
        if m:
            self.current_scenario = int(m.group(1))
            self.scenario_changes.append((elapsed, self.current_scenario, m.group(2)))
            return

        # VIOLATIONS
        m = re.match(r'VIOLATIONS,total=(\d+),scenario=(\d+)', content)
        if m:
            self.violations = int(m.group(1))
            return

        # MONITOR
        m = re.match(r'MONITOR,task=(\w+),feeds=(\d+)', content)
        if m:
            self.monitor_data[m.group(1)] = int(m.group(2))
            return

        # WDT_INIT
        m = re.match(r'WDT_INIT,timeout_sec=(\d+)', content)
        if m:
            self.wdt_timeout = int(m.group(1))
            return

        # HEAP
        m = re.match(r'HEAP,free=(\d+),min=(\d+)', content)
        if m:
            self.heap_free = int(m.group(1))
            self.heap_min = int(m.group(2))
            return


class WatchdogVisualizer:
    """Visualisasi real-time watchdog events"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = plt.figure(figsize=(16, 10))
        self.fig.suptitle('ESP32 FreeRTOS - Task Watchdog Timer Monitor',
                          fontsize=14, fontweight='bold')
        gs = GridSpec(3, 3, figure=self.fig, hspace=0.4, wspace=0.35)

        self.ax_timeline = self.fig.add_subplot(gs[0, :])
        self.ax_feeds = self.fig.add_subplot(gs[1, 0])
        self.ax_scenario = self.fig.add_subplot(gs[1, 1])
        self.ax_info = self.fig.add_subplot(gs[1, 2])
        self.ax_info.axis('off')
        self.ax_events = self.fig.add_subplot(gs[2, :2])
        self.ax_events.axis('off')
        self.ax_violations = self.fig.add_subplot(gs[2, 2])

    def update(self, frame):
        p = self.parser

        # 1. Timeline feed counts
        self.ax_timeline.clear()
        self.ax_timeline.set_title('Feed Count Timeline')
        self.ax_timeline.set_xlabel('Waktu (s)')
        self.ax_timeline.set_ylabel('Feed Count')
        colors = {'GoodTask1': 'green', 'GoodTask2': 'blue', 'BadTask': 'red'}
        for task, hist in p.feed_history.items():
            ts = list(p.timestamps)[:len(hist)]
            if ts:
                c = colors.get(task, 'gray')
                self.ax_timeline.plot(ts, list(hist), label=task, color=c, alpha=0.8)
        # Mark violations
        for vt in p.violation_times:
            self.ax_timeline.axvline(x=vt, color='red', linestyle='--', alpha=0.5)
        # Mark scenario changes
        for t, s, desc in p.scenario_changes:
            self.ax_timeline.axvline(x=t, color='orange', linestyle=':', alpha=0.4)
        self.ax_timeline.legend(fontsize=8)
        self.ax_timeline.grid(True, alpha=0.3)

        # 2. Feed counts bar
        self.ax_feeds.clear()
        self.ax_feeds.set_title('Total Feed Count')
        if p.feed_counts:
            tasks = list(p.feed_counts.keys())
            counts = [p.feed_counts[t] for t in tasks]
            cols = [colors.get(t, 'gray') for t in tasks]
            bars = self.ax_feeds.bar(tasks, counts, color=cols)
            for bar, val in zip(bars, counts):
                self.ax_feeds.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                 str(val), ha='center', va='bottom', fontsize=9)
            self.ax_feeds.tick_params(axis='x', rotation=30)

        # 3. Scenario status
        self.ax_scenario.clear()
        self.ax_scenario.set_title('Skenario Saat Ini')
        scenario_names = {1: 'Normal', 2: 'Late Feed', 3: 'Timeout!', 4: 'Recovery'}
        scenario_colors = {1: '#4ecdc4', 2: '#ffd93d', 3: '#ff6b6b', 4: '#6c5ce7'}
        sn = p.current_scenario
        name = scenario_names.get(sn, f'Skenario {sn}')
        color = scenario_colors.get(sn, 'gray')
        self.ax_scenario.barh(['Skenario'], [1], color=color, height=0.5)
        self.ax_scenario.text(0.5, 0, f'{sn}: {name}', ha='center', va='center',
                             fontsize=14, fontweight='bold')
        self.ax_scenario.set_xlim(0, 1)
        self.ax_scenario.set_xticks([])

        # 4. Info panel
        self.ax_info.clear()
        self.ax_info.axis('off')
        subs_str = '\n'.join([f"  {k}: {v}" for k, v in p.subscriptions.items()])
        info = (
            f"═══ WATCHDOG ═══\n"
            f"Timeout: {p.wdt_timeout}s\n"
            f"Violations: {p.violations}\n"
            f"\n═══ SUBSCRIPTIONS ═══\n"
            f"{subs_str}\n"
            f"\n═══ HEAP ═══\n"
            f"Free: {p.heap_free:,} B\n"
            f"Min:  {p.heap_min:,} B\n"
            f"\nData: {p.data_lines} lines"
        )
        self.ax_info.text(0.05, 0.95, info, transform=self.ax_info.transAxes,
                         fontsize=9, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.8))

        # 5. Event log
        self.ax_events.clear()
        self.ax_events.axis('off')
        self.ax_events.set_title('Event Log (terbaru)', fontsize=10, loc='left')
        events_text = "Time(s)   Task          Event    Detail\n"
        events_text += "─" * 55 + "\n"
        recent = list(p.events)[-15:]
        for t, task, event, detail in recent:
            marker = '⚠️' if event in ('TIMEOUT', 'BLOCK') else '✓' if event == 'FEED' else '●'
            events_text += f"{t:7.1f}s  {task:14s} {marker} {event:8s} {detail}\n"
        self.ax_events.text(0.02, 0.95, events_text, transform=self.ax_events.transAxes,
                           fontsize=8, verticalalignment='top', fontfamily='monospace',
                           bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

        # 6. Violations over time
        self.ax_violations.clear()
        self.ax_violations.set_title('Violations Timeline')
        if p.violation_times:
            self.ax_violations.scatter(p.violation_times,
                                      range(1, len(p.violation_times)+1),
                                      color='red', s=50, marker='x')
            self.ax_violations.set_xlabel('Waktu (s)')
            self.ax_violations.set_ylabel('Jumlah Kumulatif')
        self.ax_violations.grid(True, alpha=0.3)

        return []

    def run(self):
        ani = animation.FuncAnimation(self.fig, self.update, interval=1000, blit=False)
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
        print(f"Selesai membaca {parser.data_lines} data dari {filepath}")
    except FileNotFoundError:
        print(f"File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description='ESP32 Watchdog Timer Visualizer')
    ap.add_argument('--port', '-p', help='Serial port')
    ap.add_argument('--baud', '-b', type=int, default=115200)
    ap.add_argument('--file', '-f', help='File log')
    ap.add_argument('--no-gui', action='store_true')
    args = ap.parse_args()

    data_parser = WatchdogParser()

    if args.file:
        read_file(args.file, data_parser)
        if not args.no_gui and HAS_MATPLOTLIB:
            viz = WatchdogVisualizer(data_parser)
            viz.update(0)
            plt.tight_layout()
            plt.show()
        else:
            print(f"\nViolations: {data_parser.violations}")
            for t, c in data_parser.feed_counts.items():
                print(f"  {t}: {c} feeds")
    elif args.port:
        if not args.no_gui and HAS_MATPLOTLIB:
            thread = threading.Thread(target=read_serial,
                                     args=(args.port, args.baud, data_parser),
                                     daemon=True)
            thread.start()
            viz = WatchdogVisualizer(data_parser)
            viz.run()
        else:
            read_serial(args.port, args.baud, data_parser)
    else:
        print("Gunakan --port atau --file")
        print("  python debug_watchdog.py --port /dev/ttyUSB0")


if __name__ == '__main__':
    main()

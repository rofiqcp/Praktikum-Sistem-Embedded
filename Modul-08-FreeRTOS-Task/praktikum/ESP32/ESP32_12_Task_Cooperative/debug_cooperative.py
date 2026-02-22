#!/usr/bin/env python3
"""
============================================================================
Debug Parser & Visualizer - ESP32 Task Cooperative Scheduling
============================================================================
Mem-parse output serial [DATA] dari program ESP32_12_Task_Cooperative
dan menampilkan perbandingan preemptive vs cooperative vs hogging.

Fitur:
  - Grafik distribusi CPU antar worker per fase
  - Fairness index over time
  - Time slice comparison per fase
  - Worker execution count timeline
  - Starvation detection
  - Phase transition markers

Penggunaan:
  python debug_cooperative.py --port /dev/ttyUSB0
  python debug_cooperative.py --file output.log
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


class CooperativeParser:
    """Parser untuk data serial ESP32 Cooperative Scheduling"""

    def __init__(self, max_samples=300):
        self.max_samples = max_samples
        # Phase info
        self.current_phase = 'PREEMPTIVE'
        self.current_cycle = 0
        self.phase_changes = []
        # Per-worker stats
        self.worker_execs = defaultdict(int)
        self.worker_work = defaultdict(int)
        self.worker_exec_pct = defaultdict(float)
        self.worker_work_pct = defaultdict(float)
        self.worker_slice_us = defaultdict(int)
        self.worker_max_slice = defaultdict(int)
        self.worker_max_gap = defaultdict(int)
        self.worker_starved = defaultdict(bool)
        # Histories
        self.exec_history = defaultdict(lambda: deque(maxlen=max_samples))
        self.work_history = defaultdict(lambda: deque(maxlen=max_samples))
        self.timestamps = deque(maxlen=max_samples)
        # Fairness
        self.fairness_history = deque(maxlen=max_samples)
        self.fairness_timestamps = deque(maxlen=max_samples)
        self.current_fairness = 0
        # Phase summaries
        self.phase_summaries = defaultdict(lambda: defaultdict(dict))
        # Heap
        self.heap_free = 0
        self.heap_min = 0
        # Hogging info
        self.hog_task_id = -1
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

        # PHASE
        m = re.match(r'PHASE,num=(\d+),name=(\w+),cycle=(\d+)', content)
        if m:
            self.current_phase = m.group(2)
            self.current_cycle = int(m.group(3))
            self.phase_changes.append((elapsed, self.current_phase))
            return

        # HOG_TASK
        m = re.match(r'HOG_TASK,id=(\d+)', content)
        if m:
            self.hog_task_id = int(m.group(1))
            return

        # WORKER
        m = re.match(r'WORKER,id=(\d+),phase=(\w+),execs=(\d+),work=(\d+),'
                     r'slice_us=(\d+),avg_us=(\d+),core=(\d+)', content)
        if m:
            wid = int(m.group(1))
            self.worker_execs[wid] = int(m.group(3))
            self.worker_work[wid] = int(m.group(4))
            self.worker_slice_us[wid] = int(m.group(5))
            self.exec_history[wid].append(int(m.group(3)))
            self.work_history[wid].append(int(m.group(4)))
            self.timestamps.append(elapsed)
            return

        # STATS
        m = re.match(r'STATS,worker=(\d+),execs=(\d+),work=(\d+),'
                     r'exec_pct=([\d.]+),work_pct=([\d.]+),'
                     r'slice_us=(\d+),max_slice=(\d+),max_gap=(\d+),'
                     r'starved=(\d+)', content)
        if m:
            wid = int(m.group(1))
            self.worker_execs[wid] = int(m.group(2))
            self.worker_work[wid] = int(m.group(3))
            self.worker_exec_pct[wid] = float(m.group(4))
            self.worker_work_pct[wid] = float(m.group(5))
            self.worker_slice_us[wid] = int(m.group(6))
            self.worker_max_slice[wid] = int(m.group(7))
            self.worker_max_gap[wid] = int(m.group(8))
            self.worker_starved[wid] = int(m.group(9)) != 0
            return

        # FAIRNESS
        m = re.match(r'FAIRNESS,index=([\d.]+),phase=(\w+)', content)
        if m:
            self.current_fairness = float(m.group(1))
            self.fairness_history.append(self.current_fairness)
            self.fairness_timestamps.append(elapsed)
            return

        # PHASE_SUMMARY
        m = re.match(r'PHASE_SUMMARY,phase=(\w+),worker=(\d+),execs=(\d+),'
                     r'work=(\d+),pct=([\d.]+),starved=(\d+)', content)
        if m:
            phase = m.group(1)
            wid = int(m.group(2))
            self.phase_summaries[phase][wid] = {
                'execs': int(m.group(3)),
                'work': int(m.group(4)),
                'pct': float(m.group(5)),
                'starved': int(m.group(6)) != 0
            }
            return

        # HEAP
        m = re.match(r'HEAP,free=(\d+),min=(\d+)', content)
        if m:
            self.heap_free = int(m.group(1))
            self.heap_min = int(m.group(2))
            return


class CooperativeVisualizer:
    """Visualisasi cooperative vs preemptive scheduling"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = plt.figure(figsize=(16, 10))
        self.fig.suptitle('ESP32 FreeRTOS - Cooperative vs Preemptive Scheduling',
                          fontsize=14, fontweight='bold')
        gs = GridSpec(3, 3, figure=self.fig, hspace=0.45, wspace=0.35)

        self.ax_work = self.fig.add_subplot(gs[0, :2])
        self.ax_pct = self.fig.add_subplot(gs[0, 2])
        self.ax_fairness = self.fig.add_subplot(gs[1, 0])
        self.ax_slice = self.fig.add_subplot(gs[1, 1])
        self.ax_info = self.fig.add_subplot(gs[1, 2])
        self.ax_info.axis('off')
        self.ax_phase = self.fig.add_subplot(gs[2, :2])
        self.ax_phase.axis('off')
        self.ax_starve = self.fig.add_subplot(gs[2, 2])

    def update(self, frame):
        p = self.parser
        worker_colors = {0: '#ff6b6b', 1: '#4ecdc4', 2: '#6c5ce7'}

        # 1. Work units over time
        self.ax_work.clear()
        self.ax_work.set_title('Worker Execution Count Over Time')
        self.ax_work.set_xlabel('Sampel')
        self.ax_work.set_ylabel('Execution Count')
        for wid in range(3):
            if p.exec_history[wid]:
                c = worker_colors.get(wid, 'gray')
                self.ax_work.plot(list(p.exec_history[wid]), color=c,
                                label=f'Worker {wid}', alpha=0.8)
        # Phase change markers
        for t, phase in p.phase_changes:
            pass  # Could add vertical lines if we had sample-indexed timestamps
        self.ax_work.legend(fontsize=8)
        self.ax_work.grid(True, alpha=0.3)

        # 2. CPU distribution pie/bar
        self.ax_pct.clear()
        self.ax_pct.set_title(f'CPU Distribution ({p.current_phase})')
        if any(p.worker_work_pct.values()):
            workers = sorted(p.worker_work_pct.keys())
            pcts = [p.worker_work_pct[w] for w in workers]
            labels = [f'W{w}: {p.worker_work_pct[w]:.1f}%' for w in workers]
            colors = [worker_colors.get(w, 'gray') for w in workers]
            if sum(pcts) > 0:
                self.ax_pct.pie(pcts, labels=labels, colors=colors,
                               startangle=90, textprops={'fontsize': 8})

        # 3. Fairness over time
        self.ax_fairness.clear()
        self.ax_fairness.set_title("Jain's Fairness Index")
        self.ax_fairness.set_xlabel('Waktu (s)')
        self.ax_fairness.set_ylabel('Fairness (0-1)')
        self.ax_fairness.set_ylim(0, 1.1)
        if p.fairness_timestamps and p.fairness_history:
            ts = list(p.fairness_timestamps)
            fh = list(p.fairness_history)
            self.ax_fairness.plot(ts[:len(fh)], fh, 'b-o', markersize=3)
            self.ax_fairness.axhline(y=1.0, color='green', linestyle='--', alpha=0.5,
                                    label='Perfect (1.0)')
            self.ax_fairness.axhline(y=0.5, color='red', linestyle='--', alpha=0.5,
                                    label='Unfair (<0.5)')
            # Color background by phase
            for i, (t, phase) in enumerate(p.phase_changes):
                end_t = p.phase_changes[i+1][0] if i+1 < len(p.phase_changes) else ts[-1] if ts else t+1
                color = {'PREEMPTIVE': '#4ecdc422', 'COOPERATIVE': '#ffd93d22',
                         'HOGGING': '#ff6b6b22'}.get(phase, '#ffffff22')
                self.ax_fairness.axvspan(t, end_t, facecolor=color)
        self.ax_fairness.legend(fontsize=7)
        self.ax_fairness.grid(True, alpha=0.3)

        # 4. Time slice comparison
        self.ax_slice.clear()
        self.ax_slice.set_title('Last Time Slice (μs)')
        if any(p.worker_slice_us.values()):
            workers = sorted(p.worker_slice_us.keys())
            slices = [p.worker_slice_us[w] for w in workers]
            labels = [f'W{w}' for w in workers]
            colors = [worker_colors.get(w, 'gray') for w in workers]
            bars = self.ax_slice.bar(labels, slices, color=colors)
            for bar, val in zip(bars, slices):
                self.ax_slice.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                  f'{val:,}', ha='center', va='bottom', fontsize=8)

        # 5. Info panel
        self.ax_info.clear()
        self.ax_info.axis('off')
        hog_info = f"Hog Task: Worker{p.hog_task_id}" if p.hog_task_id >= 0 else "N/A"
        starved = [f"W{w}" for w, s in p.worker_starved.items() if s]
        starved_str = ', '.join(starved) if starved else 'None'
        info = (
            f"═══ SCHEDULING ═══\n"
            f"Phase: {p.current_phase}\n"
            f"Cycle: {p.current_cycle}\n"
            f"{hog_info}\n"
            f"\n═══ FAIRNESS ═══\n"
            f"Index: {p.current_fairness:.4f}\n"
            f"Starved: {starved_str}\n"
            f"\n═══ HEAP ═══\n"
            f"Free: {p.heap_free:,} B\n"
            f"Min:  {p.heap_min:,} B\n"
            f"\nData: {p.data_lines}"
        )
        self.ax_info.text(0.05, 0.95, info, transform=self.ax_info.transAxes,
                         fontsize=9, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.8))

        # 6. Phase summaries table
        self.ax_phase.clear()
        self.ax_phase.axis('off')
        self.ax_phase.set_title('Phase Summaries', fontsize=10, loc='left')
        summary_text = f"{'Phase':<14} {'Worker':<8} {'Work':<10} {'%':<8} {'Starved'}\n"
        summary_text += "─" * 55 + "\n"
        for phase in ['PREEMPTIVE', 'COOPERATIVE', 'HOGGING']:
            if phase in p.phase_summaries:
                for wid in sorted(p.phase_summaries[phase].keys()):
                    d = p.phase_summaries[phase][wid]
                    marker = '⚠️' if d.get('starved') else '✓'
                    summary_text += (f"{phase:<14} W{wid:<7} {d.get('work', 0):<10} "
                                   f"{d.get('pct', 0):<8.1f} {marker}\n")
        self.ax_phase.text(0.02, 0.95, summary_text, transform=self.ax_phase.transAxes,
                          fontsize=8, verticalalignment='top', fontfamily='monospace',
                          bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

        # 7. Starvation indicator
        self.ax_starve.clear()
        self.ax_starve.set_title('Max Response Gap (μs)')
        if any(p.worker_max_gap.values()):
            workers = sorted(p.worker_max_gap.keys())
            gaps = [p.worker_max_gap[w] for w in workers]
            labels = [f'W{w}' for w in workers]
            colors = ['#ff6b6b' if p.worker_starved.get(w, False) else '#4ecdc4' for w in workers]
            bars = self.ax_starve.bar(labels, gaps, color=colors)
            for bar, val in zip(bars, gaps):
                self.ax_starve.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                   f'{val:,}', ha='center', va='bottom', fontsize=7)

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
        print(f"Selesai: {parser.data_lines} data lines")
    except FileNotFoundError:
        print(f"File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description='ESP32 Cooperative Scheduling Visualizer')
    ap.add_argument('--port', '-p', help='Serial port')
    ap.add_argument('--baud', '-b', type=int, default=115200)
    ap.add_argument('--file', '-f', help='File log')
    ap.add_argument('--no-gui', action='store_true')
    args = ap.parse_args()

    data_parser = CooperativeParser()

    if args.file:
        read_file(args.file, data_parser)
        if not args.no_gui and HAS_MATPLOTLIB:
            viz = CooperativeVisualizer(data_parser)
            viz.update(0)
            plt.tight_layout()
            plt.show()
        else:
            print(f"\nPhase: {data_parser.current_phase}")
            print(f"Fairness: {data_parser.current_fairness:.4f}")
            for w, e in data_parser.worker_execs.items():
                print(f"  Worker{w}: {e} execs, starved={data_parser.worker_starved.get(w, False)}")
    elif args.port:
        if not args.no_gui and HAS_MATPLOTLIB:
            thread = threading.Thread(target=read_serial,
                                     args=(args.port, args.baud, data_parser),
                                     daemon=True)
            thread.start()
            viz = CooperativeVisualizer(data_parser)
            viz.run()
        else:
            read_serial(args.port, args.baud, data_parser)
    else:
        print("Gunakan --port atau --file")
        print("  python debug_cooperative.py --port /dev/ttyUSB0")


if __name__ == '__main__':
    main()

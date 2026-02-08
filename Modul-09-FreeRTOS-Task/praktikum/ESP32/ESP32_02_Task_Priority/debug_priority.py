#!/usr/bin/env python3
"""
Debug Serial Parser & Visualizer - ESP32_02_Task_Priority
Mem-parse output [DATA] dari serial dan menampilkan visualisasi
prioritas task, preemption, dan perubahan prioritas runtime.

Usage:
    python debug_priority.py --port /dev/ttyUSB0
    python debug_priority.py --file capture.log
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
    print("[WARN] matplotlib tidak tersedia. Install: pip install matplotlib")


class PriorityParser:
    """Parser untuk data serial ESP32 Task Priority"""

    def __init__(self):
        self.prio_starts = defaultdict(list)   # {level: [(ts, round, prio)]}
        self.prio_ends = defaultdict(list)      # {level: [(ts, round, duration_us, core)]}
        self.monitor_data = []                  # (ts, report, low_cnt, med_cnt, high_cnt)
        self.priority_history = []              # (ts, low_p, med_p, high_p)
        self.phase_events = []                  # (phase_name, ts)
        self.heap_data = []                     # (ts, free_heap)
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
            if tag == "INIT" and len(parts) >= 6:
                self.init_info = {
                    'timestamp': int(parts[1]),
                    'low_prio': int(parts[2]),
                    'med_prio': int(parts[3]),
                    'high_prio': int(parts[4]),
                    'iterations': int(parts[5])
                }
                print(f"[INIT] Priorities: L={parts[2]}, M={parts[3]}, H={parts[4]}")

            elif tag == "PRIO_START" and len(parts) >= 5:
                level = parts[1]
                ts = int(parts[2])
                rnd = int(parts[3])
                prio = int(parts[4])
                self.prio_starts[level].append((ts, rnd, prio))

            elif tag == "PRIO_END" and len(parts) >= 6:
                level = parts[1]
                ts = int(parts[2])
                rnd = int(parts[3])
                dur = int(parts[4])
                core = int(parts[5])
                self.prio_ends[level].append((ts, rnd, dur, core))

            elif tag == "MONITOR" and len(parts) >= 6:
                ts = int(parts[1])
                report = int(parts[2])
                low_c = int(parts[3])
                med_c = int(parts[4])
                high_c = int(parts[5])
                self.monitor_data.append((ts, report, low_c, med_c, high_c))

            elif tag == "PRIORITIES" and len(parts) >= 5:
                ts = int(parts[1])
                lp, mp, hp = int(parts[2]), int(parts[3]), int(parts[4])
                self.priority_history.append((ts, lp, mp, hp))

            elif tag == "PHASE" and len(parts) >= 3:
                phase = parts[1]
                ts = int(parts[2])
                self.phase_events.append((phase, ts))
                print(f"[PHASE] {phase} at {ts}ms")

            elif tag == "HEAP" and len(parts) >= 3:
                ts = int(parts[1])
                heap = int(parts[2])
                self.heap_data.append((ts, heap))

        except (ValueError, IndexError):
            pass

    def print_summary(self):
        print("\n" + "=" * 60)
        print("RINGKASAN DATA TASK PRIORITY")
        print("=" * 60)

        for level in ['LOW', 'MED', 'HIGH']:
            ends = self.prio_ends.get(level, [])
            if ends:
                durations = [e[2] for e in ends]
                avg_dur = sum(durations) / len(durations)
                min_dur = min(durations)
                max_dur = max(durations)
                cores = [e[3] for e in ends]
                print(f"\n{level} Priority Task:")
                print(f"  Rounds: {len(ends)}")
                print(f"  Duration: avg={avg_dur:.0f}us, min={min_dur}us, max={max_dur}us")
                print(f"  Cores used: {set(cores)}")

        if self.monitor_data:
            last = self.monitor_data[-1]
            print(f"\nFinal work counts: LOW={last[2]}, MED={last[3]}, HIGH={last[4]}")

        print(f"Phase events: {len(self.phase_events)}")
        print("=" * 60)

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            return

        fig = plt.figure(figsize=(15, 12))
        fig.suptitle("ESP32 FreeRTOS Task Priority Analysis", fontsize=14, fontweight='bold')
        gs = GridSpec(3, 2, figure=fig, hspace=0.4, wspace=0.3)

        colors = {'LOW': '#2196F3', 'MED': '#FF9800', 'HIGH': '#F44336'}

        # 1. Execution Duration per Priority
        ax1 = fig.add_subplot(gs[0, :])
        for level in ['LOW', 'MED', 'HIGH']:
            ends = self.prio_ends.get(level, [])
            if ends:
                ts = [e[0] / 1000.0 for e in ends]
                dur = [e[2] / 1000.0 for e in ends]  # ms
                ax1.scatter(ts, dur, label=f'{level} Priority', color=colors[level],
                           alpha=0.6, s=20)
        # Phase markers
        for phase, t in self.phase_events:
            ax1.axvline(x=t / 1000.0, color='gray', linestyle='--', alpha=0.5)
            ax1.text(t / 1000.0, ax1.get_ylim()[1] if ax1.get_ylim()[1] > 0 else 10,
                    phase.replace('_', '\n'), fontsize=7, ha='center', va='bottom')
        ax1.set_xlabel("Waktu (detik)")
        ax1.set_ylabel("Durasi Eksekusi (ms)")
        ax1.set_title("Execution Duration per Priority Level")
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # 2. Work Count Progress
        ax2 = fig.add_subplot(gs[1, 0])
        if self.monitor_data:
            m_ts = [d[0] / 1000.0 for d in self.monitor_data]
            ax2.plot(m_ts, [d[2] for d in self.monitor_data], '-', color=colors['LOW'],
                    label='LOW', linewidth=2)
            ax2.plot(m_ts, [d[3] for d in self.monitor_data], '-', color=colors['MED'],
                    label='MED', linewidth=2)
            ax2.plot(m_ts, [d[4] for d in self.monitor_data], '-', color=colors['HIGH'],
                    label='HIGH', linewidth=2)
        ax2.set_xlabel("Waktu (detik)")
        ax2.set_ylabel("Work Count")
        ax2.set_title("Cumulative Work per Priority")
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        # 3. Priority Changes Over Time
        ax3 = fig.add_subplot(gs[1, 1])
        if self.priority_history:
            p_ts = [d[0] / 1000.0 for d in self.priority_history]
            ax3.step(p_ts, [d[1] for d in self.priority_history], where='post',
                    label='LOW task', color=colors['LOW'], linewidth=2)
            ax3.step(p_ts, [d[2] for d in self.priority_history], where='post',
                    label='MED task', color=colors['MED'], linewidth=2)
            ax3.step(p_ts, [d[3] for d in self.priority_history], where='post',
                    label='HIGH task', color=colors['HIGH'], linewidth=2)
        ax3.set_xlabel("Waktu (detik)")
        ax3.set_ylabel("Priority Level")
        ax3.set_title("Runtime Priority Changes")
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # 4. Duration Distribution (Box Plot)
        ax4 = fig.add_subplot(gs[2, 0])
        box_data = []
        box_labels = []
        for level in ['LOW', 'MED', 'HIGH']:
            ends = self.prio_ends.get(level, [])
            if ends:
                box_data.append([e[2] / 1000.0 for e in ends])
                box_labels.append(level)
        if box_data:
            bp = ax4.boxplot(box_data, labels=box_labels, patch_artist=True)
            for patch, level in zip(bp['boxes'], box_labels):
                patch.set_facecolor(colors[level])
                patch.set_alpha(0.5)
        ax4.set_ylabel("Durasi (ms)")
        ax4.set_title("Execution Duration Distribution")
        ax4.grid(True, alpha=0.3, axis='y')

        # 5. Core Usage per Priority
        ax5 = fig.add_subplot(gs[2, 1])
        for i, level in enumerate(['LOW', 'MED', 'HIGH']):
            ends = self.prio_ends.get(level, [])
            if ends:
                cores = [e[3] for e in ends]
                c0 = cores.count(0)
                c1 = cores.count(1)
                ax5.bar(i - 0.15, c0, 0.3, label='Core 0' if i == 0 else '',
                       color='#66BB6A')
                ax5.bar(i + 0.15, c1, 0.3, label='Core 1' if i == 0 else '',
                       color='#42A5F5')
        ax5.set_xticks([0, 1, 2])
        ax5.set_xticklabels(['LOW', 'MED', 'HIGH'])
        ax5.set_ylabel("Eksekusi Count")
        ax5.set_title("Core Usage per Priority")
        ax5.legend()
        ax5.grid(True, alpha=0.3, axis='y')

        plt.savefig("debug_priority.png", dpi=150, bbox_inches='tight')
        print("[INFO] Grafik disimpan: debug_priority.png")
        plt.show()


def read_serial(port, baudrate, parser, duration=60):
    if serial is None:
        print("[ERROR] pyserial tidak terinstall")
        return
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[INFO] Connected to {port}. Collecting for {duration}s...")
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
    argp = argparse.ArgumentParser(description="Debug parser ESP32 Task Priority")
    argp.add_argument('--port', type=str, default=None)
    argp.add_argument('--baud', type=int, default=115200)
    argp.add_argument('--file', type=str, default=None)
    argp.add_argument('--duration', type=int, default=60)
    args = argp.parse_args()

    parser = PriorityParser()

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

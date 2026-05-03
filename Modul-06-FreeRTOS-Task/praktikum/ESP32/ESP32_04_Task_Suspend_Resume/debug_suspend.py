#!/usr/bin/env python3
"""
Debug Serial Parser & Visualizer - ESP32_04_Task_Suspend_Resume
Mem-parse output [DATA] dan menampilkan visualisasi
suspend/resume events, task states, dan ISR activity.

Usage:
    python debug_suspend.py --port /dev/ttyUSB0
    python debug_suspend.py --file capture.log
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


class SuspendResumeParser:
    """Parser untuk data serial ESP32 Task Suspend/Resume"""

    def __init__(self):
        self.blink_data = []        # (ts, led_state, count, state_str)
        self.state_changes = []     # (ts, action, count)
        self.status_data = []       # (ts, state, blinks, suspends, resumes, isr_resumes)
        self.sched_events = []      # (ts, action, round, duration, need_switch)
        self.isr_events = []        # (ts, type, data)
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
            if tag == "INIT":
                self.init_info = {'timestamp': int(parts[1])}

            elif tag == "BLINK" and len(parts) >= 5:
                ts = int(parts[1])
                led = int(parts[2])
                count = int(parts[3])
                state = parts[4]
                self.blink_data.append((ts, led, count, state))

            elif tag == "STATE_CHANGE" and len(parts) >= 4:
                action = parts[1]
                ts = int(parts[2])
                count = int(parts[3])
                self.state_changes.append((ts, action, count))
                print(f"  [{action}] at {ts}ms (count={count})")

            elif tag == "STATUS" and len(parts) >= 7:
                ts = int(parts[1])
                state = parts[2]
                blinks = int(parts[3])
                suspends = int(parts[4])
                resumes = int(parts[5])
                isr_res = int(parts[6])
                self.status_data.append((ts, state, blinks, suspends, resumes, isr_res))

            elif tag == "SCHED_DEMO" and len(parts) >= 3:
                action = parts[1]
                ts = int(parts[2])
                if action == "START":
                    rnd = int(parts[3]) if len(parts) > 3 else 0
                    self.sched_events.append((ts, 'START', rnd, 0, 0))
                elif action == "END" and len(parts) >= 6:
                    rnd = int(parts[3])
                    dur = int(parts[4])
                    ns = int(parts[5])
                    self.sched_events.append((ts, 'END', rnd, dur, ns))

            elif tag == "ISR_MODE" and len(parts) >= 3:
                self.isr_events.append((int(parts[2]), 'MODE', parts[1]))

            elif tag == "ISR_SUSPEND" and len(parts) >= 3:
                self.isr_events.append((int(parts[1]), 'SUSPEND', int(parts[2])))

            elif tag == "ISR_RESUMED" and len(parts) >= 3:
                self.isr_events.append((int(parts[1]), 'RESUMED', int(parts[2])))

        except (ValueError, IndexError):
            pass

    def print_summary(self):
        print("\n" + "=" * 60)
        print("RINGKASAN SUSPEND/RESUME DATA")
        print("=" * 60)
        print(f"Blink samples: {len(self.blink_data)}")
        print(f"State changes: {len(self.state_changes)}")

        suspends = sum(1 for _, a, _ in self.state_changes if a == "SUSPEND")
        resumes = sum(1 for _, a, _ in self.state_changes if a == "RESUME")
        print(f"  Suspends: {suspends}")
        print(f"  Resumes: {resumes}")

        print(f"Scheduler demos: {len(self.sched_events)}")
        print(f"ISR events: {len(self.isr_events)}")

        if self.status_data:
            last = self.status_data[-1]
            print(f"\nFinal status:")
            print(f"  State: {last[1]}")
            print(f"  Total blinks: {last[2]}")
            print(f"  Total suspends: {last[3]}")
            print(f"  Total resumes: {last[4]}")
            print(f"  ISR resumes: {last[5]}")
        print("=" * 60)

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            return

        fig = plt.figure(figsize=(14, 11))
        fig.suptitle("ESP32 Task Suspend/Resume Analysis", fontsize=14, fontweight='bold')
        gs = GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.3)

        # 1. Task State Timeline
        ax1 = fig.add_subplot(gs[0, :])
        if self.status_data:
            ts_list = [d[0] / 1000.0 for d in self.status_data]
            state_map = {'RUNNING': 2, 'BLOCKED': 1, 'SUSPENDED': 0, 'READY': 1.5}
            states = [state_map.get(d[1], -1) for d in self.status_data]
            ax1.step(ts_list, states, where='post', color='#2196F3', linewidth=2)
            ax1.set_yticks([0, 1, 1.5, 2])
            ax1.set_yticklabels(['SUSPENDED', 'BLOCKED', 'READY', 'RUNNING'])

        # Mark suspend/resume events
        for ts, action, _ in self.state_changes:
            color = 'red' if action == 'SUSPEND' else 'green'
            ax1.axvline(x=ts / 1000.0, color=color, linestyle='--', alpha=0.5)

        ax1.set_xlabel("Waktu (detik)")
        ax1.set_title("Blinker Task State Timeline")
        ax1.grid(True, alpha=0.3)

        # 2. Blink Count Over Time
        ax2 = fig.add_subplot(gs[1, 0])
        if self.status_data:
            ts_list = [d[0] / 1000.0 for d in self.status_data]
            blinks = [d[2] for d in self.status_data]
            ax2.plot(ts_list, blinks, 'b-', linewidth=2)
            # Highlight suspended periods
            for i in range(len(self.state_changes)):
                ts, action, _ = self.state_changes[i]
                if action == 'SUSPEND':
                    end_ts = ts + 5000  # default
                    if i + 1 < len(self.state_changes):
                        end_ts = self.state_changes[i + 1][0]
                    ax2.axvspan(ts / 1000.0, end_ts / 1000.0, alpha=0.2, color='red')
        ax2.set_xlabel("Waktu (detik)")
        ax2.set_ylabel("Blink Count")
        ax2.set_title("Blink Count (red = suspended)")
        ax2.grid(True, alpha=0.3)

        # 3. Suspend/Resume Event Counts
        ax3 = fig.add_subplot(gs[1, 1])
        if self.status_data:
            ts_list = [d[0] / 1000.0 for d in self.status_data]
            suspends = [d[3] for d in self.status_data]
            resumes = [d[4] for d in self.status_data]
            isr_res = [d[5] for d in self.status_data]
            ax3.plot(ts_list, suspends, 'r-', label='Suspends', linewidth=2)
            ax3.plot(ts_list, resumes, 'g-', label='Resumes', linewidth=2)
            ax3.plot(ts_list, isr_res, 'm--', label='ISR Resumes', linewidth=2)
        ax3.set_xlabel("Waktu (detik)")
        ax3.set_ylabel("Count")
        ax3.set_title("Cumulative Suspend/Resume Counts")
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # 4. Scheduler Suspend Duration
        ax4 = fig.add_subplot(gs[2, 0])
        sched_ends = [(e[0], e[3]) for e in self.sched_events if e[1] == 'END']
        if sched_ends:
            ts_list = [e[0] / 1000.0 for e in sched_ends]
            durations = [e[1] / 1000.0 for e in sched_ends]
            ax4.bar(ts_list, durations, width=0.5, color='#FF9800', alpha=0.7)
        ax4.set_xlabel("Waktu (detik)")
        ax4.set_ylabel("Durasi (ms)")
        ax4.set_title("Scheduler Suspend Duration")
        ax4.grid(True, alpha=0.3, axis='y')

        # 5. Event Timeline
        ax5 = fig.add_subplot(gs[2, 1])
        event_types = {'SUSPEND': 0, 'RESUME': 1, 'ISR': 2, 'SCHED': 3}
        colors_map = {'SUSPEND': 'red', 'RESUME': 'green', 'ISR': 'purple', 'SCHED': 'orange'}

        for ts, action, _ in self.state_changes:
            y = event_types.get(action, 0)
            c = colors_map.get(action, 'gray')
            ax5.scatter(ts / 1000.0, y, color=c, s=50, zorder=5)

        for ts, etype, _ in self.isr_events:
            ax5.scatter(ts / 1000.0, 2, color='purple', s=50, marker='^', zorder=5)

        for e in self.sched_events:
            ax5.scatter(e[0] / 1000.0, 3, color='orange', s=50, marker='D', zorder=5)

        ax5.set_yticks([0, 1, 2, 3])
        ax5.set_yticklabels(['Suspend', 'Resume', 'ISR', 'Scheduler'])
        ax5.set_xlabel("Waktu (detik)")
        ax5.set_title("Event Timeline")
        ax5.grid(True, alpha=0.3)

        plt.savefig("debug_suspend.png", dpi=150, bbox_inches='tight')
        print("[INFO] Grafik disimpan: debug_suspend.png")
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
    argp = argparse.ArgumentParser(description="Debug parser ESP32 Suspend/Resume")
    argp.add_argument('--port', type=str, default=None)
    argp.add_argument('--baud', type=int, default=115200)
    argp.add_argument('--file', type=str, default=None)
    argp.add_argument('--duration', type=int, default=60)
    args = argp.parse_args()

    parser = SuspendResumeParser()

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

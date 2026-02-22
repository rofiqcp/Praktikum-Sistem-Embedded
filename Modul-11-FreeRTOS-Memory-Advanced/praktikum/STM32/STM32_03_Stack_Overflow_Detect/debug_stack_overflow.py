#!/usr/bin/env python3
"""
debug_stack_overflow.py
=======================
Monitors serial output from STM32_03_Stack_Overflow_Detect.
Tracks stack watermarks, detects overflow events, and visualises results.

Usage:
    python debug_stack_overflow.py --port /dev/ttyUSB0
    python debug_stack_overflow.py --port COM3 --plot
    python debug_stack_overflow.py --port /dev/ttyUSB0 --csv stack_log.csv
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime
from collections import defaultdict, deque

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    print("WARNING: matplotlib not installed. Plotting disabled.")
    HAS_MATPLOTLIB = False


# ── Regex patterns ─────────────────────────────────────────────────────────
RE_WMARK_ENTRY = re.compile(
    r'\[WMARK\]\s+(\w+)\s*:\s*(\d+)\s*words\s*remaining'
)
RE_OVERFLOW_COUNT = re.compile(
    r'\[WMARK\]\s+Overflow count:\s*(\d+)'
)
RE_SAFE = re.compile(
    r'\[SAFE\]\s+.*Watermark=(\d+)\s*words'
)
RE_RECURSE = re.compile(
    r'\[RECURSE\]\s+Depth=(\d+)\s*\|\s*Watermark=(\d+)'
)
RE_BIGLOCAL = re.compile(
    r'\[BIGLOCAL\]\s+Step\s+(\d+):\s*(\d+)-byte\s+local\s+.*Watermark=(\d+)'
)
RE_OVERFLOW_DETECT = re.compile(
    r'\[OVERFLOW\]\s+Stack overflow detected in task:\s*(\w+)'
)
RE_PHASE = re.compile(
    r'\[WMARK\].*Phase:\s*(\d+)'
)


class StackMonitor:
    """Tracks stack watermarks and overflow events."""

    def __init__(self, csv_path="stack_overflow_log.csv", max_points=500):
        self.csv_path = csv_path
        self.max_points = max_points

        # Per-task watermark history: task_name -> [(time, watermark)]
        self.watermarks = defaultdict(lambda: deque(maxlen=max_points))

        # Overflow events
        self.overflows = []

        # Recursion depth tracking
        self.recursion_data = []  # (time, depth, watermark)

        # BigLocal step tracking
        self.biglocal_data = []  # (time, step, local_size, watermark)

        # Phase tracking
        self.current_phase = 0
        self.phase_changes = []

        self.start_time = time.time()
        self._init_csv()

    def _init_csv(self):
        with open(self.csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'timestamp', 'elapsed_s', 'event', 'task_name',
                'watermark_words', 'depth', 'local_size', 'phase', 'detail'
            ])
        print(f"[CSV] Logging to {self.csv_path}")

    def _log(self, event, task='', watermark='', depth='', local_size='',
             phase='', detail=''):
        elapsed = time.time() - self.start_time
        now_str = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        with open(self.csv_path, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                now_str, f"{elapsed:.2f}", event, task,
                watermark, depth, local_size, phase, detail
            ])

    def process_line(self, line):
        """Parse a serial line and update tracking."""
        elapsed = time.time() - self.start_time

        # ── Watermark report entries ──
        m = RE_WMARK_ENTRY.search(line)
        if m:
            task_name = m.group(1)
            watermark = int(m.group(2))
            self.watermarks[task_name].append((elapsed, watermark))
            self._log('WATERMARK', task_name, watermark)

            # Alert if watermark is critically low
            if watermark < 10:
                print(f"  *** WARNING: {task_name} watermark critically low: "
                      f"{watermark} words! ***")
            return

        # ── Overflow count ──
        m = RE_OVERFLOW_COUNT.search(line)
        if m:
            count = int(m.group(1))
            self._log('OVERFLOW_COUNT', detail=f"count={count}")
            return

        # ── Phase tracking ──
        m = RE_PHASE.search(line)
        if m:
            phase = int(m.group(1))
            if phase != self.current_phase:
                self.current_phase = phase
                self.phase_changes.append((elapsed, phase))
                self._log('PHASE_CHANGE', phase=phase)
            return

        # ── Recursion depth ──
        m = RE_RECURSE.search(line)
        if m:
            depth = int(m.group(1))
            watermark = int(m.group(2))
            self.recursion_data.append((elapsed, depth, watermark))
            self._log('RECURSION', 'Recurse', watermark, depth)
            if watermark < 5:
                print(f"  *** DANGER: Recursion depth {depth}, "
                      f"watermark {watermark} words! ***")
            return

        # ── BigLocal steps ──
        m = RE_BIGLOCAL.search(line)
        if m:
            step = int(m.group(1))
            local_size = int(m.group(2))
            watermark = int(m.group(3))
            self.biglocal_data.append((elapsed, step, local_size, watermark))
            self._log('BIGLOCAL', 'BigLocal', watermark, local_size=local_size,
                      detail=f"step={step}")
            return

        # ── Overflow detection ──
        m = RE_OVERFLOW_DETECT.search(line)
        if m:
            task_name = m.group(1)
            self.overflows.append((elapsed, task_name))
            self._log('OVERFLOW', task_name, detail='STACK OVERFLOW DETECTED')
            print(f"\n{'!'*60}")
            print(f"  STACK OVERFLOW DETECTED in task: {task_name}")
            print(f"  Time: {elapsed:.2f}s")
            print(f"{'!'*60}\n")
            return

        # ── Safe task watermark ──
        m = RE_SAFE.search(line)
        if m:
            watermark = int(m.group(1))
            self.watermarks['Safe'].append((elapsed, watermark))
            self._log('WATERMARK', 'Safe', watermark)
            return

    def print_summary(self):
        """Print text summary."""
        print("\n" + "=" * 60)
        print("  Stack Overflow Detection Summary")
        print("=" * 60)

        print(f"\n  Overflow events: {len(self.overflows)}")
        for t, task in self.overflows:
            print(f"    [{t:.2f}s] Task: {task}")

        print(f"\n  Tasks monitored: {len(self.watermarks)}")
        for task, data in self.watermarks.items():
            if data:
                wmarks = [d[1] for d in data]
                print(f"    {task:12s}: min={min(wmarks):3d}, "
                      f"max={max(wmarks):3d}, last={wmarks[-1]:3d} words")

        if self.recursion_data:
            depths = [d[1] for d in self.recursion_data]
            wmarks = [d[2] for d in self.recursion_data]
            print(f"\n  Recursion test:")
            print(f"    Max depth reached : {max(depths)}")
            print(f"    Min watermark     : {min(wmarks)} words")

        if self.biglocal_data:
            print(f"\n  BigLocal test steps: {len(self.biglocal_data)}")
            for _, step, lsz, wm in self.biglocal_data:
                print(f"    Step {step}: {lsz}-byte local, watermark={wm} words")

        print("=" * 60)

    def plot_summary(self):
        """Generate visualisation."""
        if not HAS_MATPLOTLIB:
            return

        num_plots = 2 + (1 if self.recursion_data else 0) + (1 if self.biglocal_data else 0)
        fig, axes = plt.subplots(num_plots, 1, figsize=(14, 4 * num_plots))
        if num_plots == 1:
            axes = [axes]
        fig.suptitle('Stack Overflow Detection – STM32_03', fontsize=14)

        plot_idx = 0

        # Plot 1: Watermarks over time for all tasks
        ax = axes[plot_idx]
        colors = ['#2ecc71', '#3498db', '#e74c3c', '#f39c12', '#9b59b6']
        ci = 0
        for task, data in self.watermarks.items():
            if data:
                t = [d[0] for d in data]
                w = [d[1] for d in data]
                ax.plot(t, w, '-o', markersize=3, label=task,
                        color=colors[ci % len(colors)])
                ci += 1
        ax.axhline(y=0, color='red', linestyle='--', linewidth=2,
                   label='Overflow threshold')
        ax.set_ylabel('Watermark (words)')
        ax.set_title('Stack High-Water Marks Over Time')
        ax.legend(loc='upper right')
        ax.grid(True, alpha=0.3)

        # Mark overflow events
        for t, task in self.overflows:
            ax.axvline(x=t, color='red', linewidth=2, alpha=0.7)
            ax.annotate(f'OVERFLOW\n{task}', xy=(t, 0),
                        fontsize=8, color='red', fontweight='bold',
                        ha='center', va='bottom')
        plot_idx += 1

        # Plot 2: Phase timeline
        ax = axes[plot_idx]
        phase_labels = {0: 'Safe only', 1: 'Recursive task', 2: 'BigLocal task'}
        for i, (t, p) in enumerate(self.phase_changes):
            end_t = (self.phase_changes[i + 1][0]
                     if i + 1 < len(self.phase_changes)
                     else (time.time() - self.start_time))
            color = ['#2ecc71', '#e67e22', '#e74c3c'][p % 3]
            ax.barh(0, end_t - t, left=t, height=0.5, color=color,
                    alpha=0.7, edgecolor='black')
            ax.text((t + end_t) / 2, 0, phase_labels.get(p, f'Phase {p}'),
                    ha='center', va='center', fontsize=9)
        ax.set_yticks([])
        ax.set_xlabel('Time (s)')
        ax.set_title('Test Phase Timeline')
        ax.grid(True, alpha=0.3, axis='x')
        plot_idx += 1

        # Plot 3: Recursion depth vs watermark
        if self.recursion_data:
            ax = axes[plot_idx]
            depths = [d[1] for d in self.recursion_data]
            wmarks = [d[2] for d in self.recursion_data]
            ax.plot(depths, wmarks, 'r-o', markersize=4)
            ax.set_xlabel('Recursion Depth')
            ax.set_ylabel('Stack Watermark (words)')
            ax.set_title('Stack Consumption vs Recursion Depth')
            ax.axhline(y=0, color='red', linestyle='--', linewidth=2)
            ax.grid(True, alpha=0.3)
            plot_idx += 1

        # Plot 4: BigLocal bar chart
        if self.biglocal_data:
            ax = axes[plot_idx]
            steps = [f"Step {d[1]}\n{d[2]}B" for d in self.biglocal_data]
            wmarks = [d[3] for d in self.biglocal_data]
            bars = ax.bar(steps, wmarks, color=['#2ecc71', '#f39c12', '#e74c3c'],
                          alpha=0.8, edgecolor='black')
            ax.set_ylabel('Watermark (words)')
            ax.set_title('BigLocal Task: Watermark vs Local Variable Size')
            ax.axhline(y=0, color='red', linestyle='--', linewidth=2)
            for bar, wm in zip(bars, wmarks):
                ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height(),
                        f'{wm}', ha='center', va='bottom', fontweight='bold')
            ax.grid(True, alpha=0.3, axis='y')
            plot_idx += 1

        plt.tight_layout()
        png_path = self.csv_path.replace('.csv', '.png')
        plt.savefig(png_path, dpi=150)
        print(f"[PLOT] Saved to {png_path}")
        plt.show()


def auto_detect_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or '').lower()
        if any(k in desc for k in ['usb', 'uart', 'serial', 'ch340', 'cp210', 'ftdi']):
            return p.device
    if ports:
        return ports[0].device
    return None


def main():
    parser = argparse.ArgumentParser(
        description='FreeRTOS Stack Overflow Detection Debug Tool')
    parser.add_argument('--port', type=str, default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--csv', type=str, default='stack_overflow_log.csv',
                        help='CSV log file')
    parser.add_argument('--duration', type=int, default=0,
                        help='Run duration in seconds (0=forever)')
    parser.add_argument('--plot', action='store_true',
                        help='Show plot after capture')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    if port is None:
        print("ERROR: No serial port found. Use --port to specify.")
        sys.exit(1)

    monitor = StackMonitor(csv_path=args.csv)

    print(f"[SERIAL] Opening {port} @ {args.baud} baud")
    try:
        ser = serial.Serial(port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"ERROR: Cannot open {port}: {e}")
        sys.exit(1)

    print("[SERIAL] Connected. Press Ctrl+C to stop.\n")
    start = time.time()

    try:
        while True:
            if args.duration > 0 and (time.time() - start) > args.duration:
                print(f"\n[INFO] Duration {args.duration}s reached.")
                break

            raw = ser.readline()
            if not raw:
                continue

            try:
                line = raw.decode('utf-8', errors='replace').strip()
            except Exception:
                continue

            if line:
                print(line)
                monitor.process_line(line)

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    finally:
        ser.close()

    monitor.print_summary()

    if args.plot:
        monitor.plot_summary()


if __name__ == '__main__':
    main()

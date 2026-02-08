#!/usr/bin/env python3
"""
Debug Serial Parser & Visualizer - ESP32_03_Task_Delay_Periodic
Mem-parse output [DATA] dan menampilkan perbandingan timing
vTaskDelay() vs vTaskDelayUntil() dengan analisis jitter.

Usage:
    python debug_delay.py --port /dev/ttyUSB0
    python debug_delay.py --file capture.log
"""

import sys
import argparse
import time
import math
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
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia")


class DelayParser:
    """Parser untuk data serial ESP32 Task Delay Periodic"""

    def __init__(self):
        self.delay_data = []       # (ts, cycle, period_us, mean, stddev, max_dev)
        self.periodic_data = []    # (ts, cycle, period_us, mean, stddev, max_dev)
        self.compare_data = []     # (ts, d_samples, p_samples, d_mean, p_mean, d_std, p_std, d_maxdev, p_maxdev)
        self.jitter_warnings = []  # (ts, type, deviation)
        self.heap_data = []        # (ts, heap)
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
            if tag == "INIT" and len(parts) >= 5:
                self.init_info = {
                    'target_ms': int(parts[1]) if len(parts) > 1 else 100,
                    'work_ms': int(parts[2]) if len(parts) > 2 else 15,
                    'tick_hz': int(parts[3]) if len(parts) > 3 else 100
                }

            elif tag == "DELAY_TASK" and len(parts) >= 7:
                ts = int(parts[1])
                cycle = int(parts[2])
                period = int(parts[3])
                mean = float(parts[4])
                std = float(parts[5])
                maxdev = int(parts[6])
                self.delay_data.append((ts, cycle, period, mean, std, maxdev))

            elif tag == "PERIODIC_TASK" and len(parts) >= 7:
                ts = int(parts[1])
                cycle = int(parts[2])
                period = int(parts[3])
                mean = float(parts[4])
                std = float(parts[5])
                maxdev = int(parts[6])
                self.periodic_data.append((ts, cycle, period, mean, std, maxdev))

            elif tag == "COMPARE" and len(parts) >= 10:
                ts = int(parts[1])
                d_samp = int(parts[2])
                p_samp = int(parts[3])
                d_mean = float(parts[4])
                p_mean = float(parts[5])
                d_std = float(parts[6])
                p_std = float(parts[7])
                d_maxdev = int(parts[8])
                p_maxdev = int(parts[9])
                self.compare_data.append((ts, d_samp, p_samp, d_mean, p_mean,
                                         d_std, p_std, d_maxdev, p_maxdev))

            elif tag == "JITTER_WARN" and len(parts) >= 4:
                jtype = parts[1]
                ts = int(parts[2])
                dev = int(parts[3])
                self.jitter_warnings.append((ts, jtype, dev))

            elif tag == "HEAP" and len(parts) >= 3:
                self.heap_data.append((int(parts[1]), int(parts[2])))

        except (ValueError, IndexError):
            pass

    def print_summary(self):
        print("\n" + "=" * 60)
        print("RINGKASAN PERBANDINGAN DELAY")
        print("=" * 60)

        target_us = self.init_info.get('target_ms', 100) * 1000

        if self.delay_data:
            periods = [d[2] for d in self.delay_data if d[2] > 0]
            if periods:
                avg = sum(periods) / len(periods)
                drift_pct = ((avg - target_us) / target_us) * 100
                print(f"\nvTaskDelay():")
                print(f"  Samples: {len(periods)}")
                print(f"  Average period: {avg:.1f} us (target: {target_us} us)")
                print(f"  Drift: {drift_pct:.2f}%")
                print(f"  Min: {min(periods)} us, Max: {max(periods)} us")
                if self.delay_data[-1][4] > 0:
                    print(f"  StdDev: {self.delay_data[-1][4]:.1f} us")

        if self.periodic_data:
            periods = [d[2] for d in self.periodic_data if d[2] > 0]
            if periods:
                avg = sum(periods) / len(periods)
                drift_pct = ((avg - target_us) / target_us) * 100
                print(f"\nvTaskDelayUntil():")
                print(f"  Samples: {len(periods)}")
                print(f"  Average period: {avg:.1f} us (target: {target_us} us)")
                print(f"  Drift: {drift_pct:.2f}%")
                print(f"  Min: {min(periods)} us, Max: {max(periods)} us")
                if self.periodic_data[-1][4] > 0:
                    print(f"  StdDev: {self.periodic_data[-1][4]:.1f} us")

        if self.compare_data:
            last = self.compare_data[-1]
            if last[5] > 0 and last[6] > 0:
                improvement = ((last[5] - last[6]) / last[5]) * 100
                print(f"\n>>> vTaskDelayUntil {improvement:.1f}% lebih presisi! <<<")

        print(f"\nJitter warnings: {len(self.jitter_warnings)}")
        print("=" * 60)

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            return

        fig = plt.figure(figsize=(15, 12))
        fig.suptitle("ESP32 vTaskDelay vs vTaskDelayUntil - Timing Analysis",
                     fontsize=14, fontweight='bold')
        gs = GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.3)

        target_us = self.init_info.get('target_ms', 100) * 1000

        # 1. Period Over Time - Both Methods
        ax1 = fig.add_subplot(gs[0, :])
        if self.delay_data:
            d_ts = [d[0] / 1000.0 for d in self.delay_data if d[2] > 0]
            d_per = [d[2] for d in self.delay_data if d[2] > 0]
            ax1.plot(d_ts, d_per, 'r-', label='vTaskDelay()', alpha=0.7, linewidth=1)
        if self.periodic_data:
            p_ts = [d[0] / 1000.0 for d in self.periodic_data if d[2] > 0]
            p_per = [d[2] for d in self.periodic_data if d[2] > 0]
            ax1.plot(p_ts, p_per, 'b-', label='vTaskDelayUntil()', alpha=0.7, linewidth=1)
        ax1.axhline(y=target_us, color='green', linestyle='--', label=f'Target ({target_us}us)')
        ax1.set_xlabel("Waktu (detik)")
        ax1.set_ylabel("Periode Aktual (us)")
        ax1.set_title("Actual Period Over Time")
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # 2. Standard Deviation (Jitter) Over Time
        ax2 = fig.add_subplot(gs[1, 0])
        if self.delay_data:
            d_ts = [d[0] / 1000.0 for d in self.delay_data if d[4] > 0]
            d_std = [d[4] for d in self.delay_data if d[4] > 0]
            ax2.plot(d_ts, d_std, 'r-o', label='vTaskDelay()', markersize=3)
        if self.periodic_data:
            p_ts = [d[0] / 1000.0 for d in self.periodic_data if d[4] > 0]
            p_std = [d[4] for d in self.periodic_data if d[4] > 0]
            ax2.plot(p_ts, p_std, 'b-s', label='vTaskDelayUntil()', markersize=3)
        ax2.set_xlabel("Waktu (detik)")
        ax2.set_ylabel("Std Deviation (us)")
        ax2.set_title("Jitter (Standard Deviation) Over Time")
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        # 3. Max Deviation Over Time
        ax3 = fig.add_subplot(gs[1, 1])
        if self.delay_data:
            d_ts = [d[0] / 1000.0 for d in self.delay_data]
            d_maxdev = [d[5] for d in self.delay_data]
            ax3.plot(d_ts, d_maxdev, 'r-', label='vTaskDelay()', linewidth=2)
        if self.periodic_data:
            p_ts = [d[0] / 1000.0 for d in self.periodic_data]
            p_maxdev = [d[5] for d in self.periodic_data]
            ax3.plot(p_ts, p_maxdev, 'b-', label='vTaskDelayUntil()', linewidth=2)
        ax3.set_xlabel("Waktu (detik)")
        ax3.set_ylabel("Max Deviation (us)")
        ax3.set_title("Maximum Deviation from Target")
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # 4. Period Distribution Histogram
        ax4 = fig.add_subplot(gs[2, 0])
        if self.delay_data:
            d_periods = [d[2] for d in self.delay_data if d[2] > 0]
            if d_periods:
                ax4.hist(d_periods, bins=30, alpha=0.5, color='red', label='vTaskDelay()')
        if self.periodic_data:
            p_periods = [d[2] for d in self.periodic_data if d[2] > 0]
            if p_periods:
                ax4.hist(p_periods, bins=30, alpha=0.5, color='blue', label='vTaskDelayUntil()')
        ax4.axvline(x=target_us, color='green', linestyle='--', label='Target')
        ax4.set_xlabel("Periode (us)")
        ax4.set_ylabel("Frekuensi")
        ax4.set_title("Period Distribution")
        ax4.legend()
        ax4.grid(True, alpha=0.3)

        # 5. Comparison Bar Chart (Final Stats)
        ax5 = fig.add_subplot(gs[2, 1])
        if self.compare_data:
            last = self.compare_data[-1]
            metrics = ['Mean\nDrift (us)', 'StdDev\n(us)', 'Max Dev\n(us)']
            delay_vals = [abs(last[3] - target_us), last[5], last[7]]
            periodic_vals = [abs(last[4] - target_us), last[6], last[8]]
            x = range(len(metrics))
            w = 0.35
            ax5.bar([i - w/2 for i in x], delay_vals, w, label='vTaskDelay()',
                   color='#EF5350')
            ax5.bar([i + w/2 for i in x], periodic_vals, w, label='vTaskDelayUntil()',
                   color='#42A5F5')
            ax5.set_xticks(list(x))
            ax5.set_xticklabels(metrics)
            ax5.set_ylabel("Microseconds")
            ax5.set_title("Final Timing Comparison")
            ax5.legend()
            ax5.grid(True, alpha=0.3, axis='y')
            # Add values on bars
            for i, (dv, pv) in enumerate(zip(delay_vals, periodic_vals)):
                ax5.text(i - w/2, dv + max(delay_vals) * 0.02, f'{dv:.0f}',
                        ha='center', fontsize=8)
                ax5.text(i + w/2, pv + max(delay_vals) * 0.02, f'{pv:.0f}',
                        ha='center', fontsize=8)

        plt.savefig("debug_delay.png", dpi=150, bbox_inches='tight')
        print("[INFO] Grafik disimpan: debug_delay.png")
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
    argp = argparse.ArgumentParser(description="Debug parser ESP32 Delay Periodic")
    argp.add_argument('--port', type=str, default=None)
    argp.add_argument('--baud', type=int, default=115200)
    argp.add_argument('--file', type=str, default=None)
    argp.add_argument('--duration', type=int, default=60)
    args = argp.parse_args()

    parser = DelayParser()

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

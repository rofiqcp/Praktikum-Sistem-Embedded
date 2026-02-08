#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_08_Task_Idle_Hook
==========================================================================
 Mem-parse output serial dari program Task Idle Hook FreeRTOS.
 Mengekstrak idle counter, sleep counter, dan estimasi CPU utilization.

 Penggunaan:
   python debug_idle_hook.py --port /dev/ttyUSB0
   python debug_idle_hook.py --file capture.log
   python debug_idle_hook.py --demo

 Output:
   - Grafik CPU utilization over time
   - Idle counts per reporting period
   - Sleep mode entry statistics
==========================================================================
"""

import argparse
import sys
import re
import time
import os
from datetime import datetime
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class IdleHookParser:
    """Parser dan visualizer untuk data Idle Hook dan CPU utilization."""

    def __init__(self):
        self.monitor_data = []     # [(time, idle_total, delta_idle, sleep_total, delta_sleep, period_ms)]
        self.worker_data = []      # [(time, iteration)]
        self.cpu_usage = []        # [(time, estimated_pct)]
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()
        self.max_idle_rate = None  # Calibrated max idle/s when fully idle
        self.baseline_set = False

    def parse_line(self, line):
        self.total_lines += 1
        line = line.strip()
        if not line:
            return
        t = time.time() - self.start_time
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

        # Parse monitor output:
        # "[Monitor] Idle: 123456 (+5000), Sleep: 123400 (+4990) dalam 2000 ms"
        m = re.search(
            r'\[Monitor\]\s*Idle:\s*(\d+)\s*\(\+(\d+)\).*Sleep:\s*(\d+)\s*\(\+(\d+)\)\s*dalam\s*(\d+)\s*ms',
            line
        )
        if m:
            idle_total = int(m.group(1))
            delta_idle = int(m.group(2))
            sleep_total = int(m.group(3))
            delta_sleep = int(m.group(4))
            period_ms = int(m.group(5))
            self.monitor_data.append((t, idle_total, delta_idle, sleep_total, delta_sleep, period_ms))
            self.data_lines += 1

            # Estimate CPU utilization
            idle_per_sec = (delta_idle * 1000.0) / period_ms if period_ms > 0 else 0
            if self.max_idle_rate is None or idle_per_sec > self.max_idle_rate:
                self.max_idle_rate = idle_per_sec

            if self.max_idle_rate and self.max_idle_rate > 0:
                cpu_pct = max(0, min(100, (1.0 - idle_per_sec / self.max_idle_rate) * 100))
            else:
                cpu_pct = 0
            self.cpu_usage.append((t, cpu_pct))

            bar_len = int(cpu_pct / 5)
            bar = '█' * bar_len + '░' * (20 - bar_len)
            print(f"[{ts}] 📊 [{bar}] CPU ~{cpu_pct:.0f}%  idle +{delta_idle}, sleep +{delta_sleep}")
            return

        # Parse worker iteration
        m2 = re.search(r'\[Worker\]\s*Iterasi:\s*(\d+)', line)
        if m2:
            iteration = int(m2.group(1))
            self.worker_data.append((t, iteration))
            self.data_lines += 1
            print(f"[{ts}] ⚙️  Worker iterasi: {iteration}")
            return

        # Parse [DATA] tags if present
        dm = re.match(r'\[DATA\]\s*(\w+),(.*)', line)
        if dm:
            self.data_lines += 1
            print(f"[{ts}] 📌 [{dm.group(1)}] {dm.group(2)}")
            return

        # Generic
        if 'dimulai' in line.lower() or 'started' in line.lower():
            print(f"[{ts}] ℹ️  {line}")
        elif 'idle' in line.lower() and 'hook' in line.lower():
            print(f"[{ts}] 💤 {line}")

    def print_summary(self):
        print(f"\n{'='*55}")
        print(f" RINGKASAN IDLE HOOK")
        print(f"{'='*55}")
        print(f"  Total lines parsed : {self.total_lines}")
        print(f"  Data lines         : {self.data_lines}")
        print(f"  Monitor reports    : {len(self.monitor_data)}")
        print(f"  Worker iterations  : {len(self.worker_data)}")
        if self.monitor_data:
            last = self.monitor_data[-1]
            print(f"  Last idle total    : {last[1]}")
            print(f"  Last sleep total   : {last[3]}")
        if self.cpu_usage:
            cpus = [c[1] for c in self.cpu_usage]
            print(f"  CPU usage range    : {min(cpus):.0f}% - {max(cpus):.0f}%")
            avg_cpu = sum(cpus) / len(cpus)
            print(f"  CPU usage average  : {avg_cpu:.1f}%")
            print(f"  Max idle rate      : {self.max_idle_rate:.0f} /s (calibrated)")
        print(f"{'='*55}")

    def plot(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] matplotlib tidak tersedia, skip plot.")
            return
        if not self.monitor_data:
            print("[WARN] Tidak ada data untuk di-plot.")
            return

        fig, axes = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
        fig.suptitle('STM32_08 Idle Hook & CPU Utilization', fontsize=14)

        # Plot 1: CPU utilization
        ax1 = axes[0]
        if self.cpu_usage:
            times = [c[0] for c in self.cpu_usage]
            cpus = [c[1] for c in self.cpu_usage]
            ax1.plot(times, cpus, 'r-o', markersize=4, linewidth=2, label='CPU Usage')
            ax1.fill_between(times, cpus, alpha=0.2, color='red')
            ax1.axhline(y=50, color='orange', linestyle='--', alpha=0.5, label='50%')
        ax1.set_ylabel('CPU Usage (%)')
        ax1.set_ylim(-5, 105)
        ax1.set_title('Estimated CPU Utilization')
        ax1.grid(True, alpha=0.3)
        ax1.legend(loc='upper right', fontsize=8)

        # Plot 2: Idle counts per period
        ax2 = axes[1]
        if self.monitor_data:
            times = [m[0] for m in self.monitor_data]
            deltas = [m[2] for m in self.monitor_data]
            sleep_deltas = [m[4] for m in self.monitor_data]
            ax2.bar(times, deltas, width=0.8, alpha=0.7, color='#2196F3', label='Idle Δ')
            ax2.bar(times, sleep_deltas, width=0.4, alpha=0.7, color='#4CAF50', label='Sleep Δ')
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('Count per Period')
        ax2.set_title('Idle & Sleep Counts per Reporting Period')
        ax2.grid(True, alpha=0.3)
        ax2.legend(loc='upper right', fontsize=8)

        plt.tight_layout()
        plt.savefig('idle_hook_analysis.png', dpi=150)
        print("[INFO] Plot disimpan: idle_hook_analysis.png")
        plt.show()


def generate_demo_data():
    """Generate simulated idle hook data."""
    lines = []
    lines.append("=== Idle Hook Demo ===")
    lines.append("[Idle Hook] Task dimulai")
    lines.append("[Worker] Task dimulai, periode: 500 ms")
    lines.append("[Monitor] Task dimulai")
    lines.append("[Monitor] Periode laporan: 2000 ms")

    idle_total = 0
    sleep_total = 0
    iteration = 0
    period_ms = 2000

    # Simulate 20 reporting periods with varying load
    for i in range(20):
        # Vary idle rate to simulate CPU load changes
        if i < 5:
            delta_idle = 45000   # Low CPU usage
        elif i < 10:
            delta_idle = 20000   # Medium CPU usage
        elif i < 15:
            delta_idle = 8000    # High CPU usage
        else:
            delta_idle = 40000   # Back to low

        delta_sleep = int(delta_idle * 0.98)
        idle_total += delta_idle
        sleep_total += delta_sleep

        lines.append(
            f"[Monitor] Idle: {idle_total} (+{delta_idle}), "
            f"Sleep: {sleep_total} (+{delta_sleep}) dalam {period_ms} ms"
        )
        # Worker iterations happen between monitor reports
        for _ in range(4):
            iteration += 1
            lines.append(f"[Worker] Iterasi: {iteration} selesai")
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug parser untuk STM32_08 Task Idle Hook')
    parser.add_argument('--port', type=str, help='Serial port (e.g. /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', type=str, help='File log untuk di-parse')
    parser.add_argument('--demo', action='store_true', help='Jalankan dengan data demo')
    args = parser.parse_args()

    p = IdleHookParser()

    if args.demo:
        print("[INFO] Mode DEMO - menggunakan data simulasi\n")
        for line in generate_demo_data():
            p.parse_line(line)
            time.sleep(0.02)
        p.print_summary()
        p.plot()
    elif args.file:
        print(f"[INFO] Membaca file: {args.file}\n")
        with open(args.file, 'r', errors='replace') as f:
            for line in f:
                p.parse_line(line)
        p.print_summary()
        p.plot()
    elif args.port:
        if not HAS_SERIAL:
            print("[ERROR] pyserial belum terinstall. pip install pyserial")
            sys.exit(1)
        print(f"[INFO] Membuka {args.port} @ {args.baud} baud\n")
        try:
            ser = serial.Serial(args.port, args.baud, timeout=1)
            while True:
                raw = ser.readline()
                if raw:
                    p.parse_line(raw.decode('utf-8', errors='replace'))
        except KeyboardInterrupt:
            print("\n[INFO] Dihentikan oleh user.")
            ser.close()
            p.print_summary()
            p.plot()
        except serial.SerialException as e:
            print(f"[ERROR] Serial: {e}")
            sys.exit(1)
    else:
        parser.print_help()
        print("\nContoh: python debug_idle_hook.py --demo")


if __name__ == '__main__':
    main()

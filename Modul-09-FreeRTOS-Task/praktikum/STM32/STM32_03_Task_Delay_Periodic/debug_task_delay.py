#!/usr/bin/env python3
"""
============================================================================
Debug & Visualisasi: STM32_03_Task_Delay_Periodic
============================================================================

Parser serial output dan visualisasi data perbandingan
vTaskDelay() vs vTaskDelayUntil() pada FreeRTOS.

Menampilkan grafik:
  1. Period aktual vs target (line chart overlay)
  2. Deviation dari target (scatter plot)
  3. Drift kumulatif (line chart - kunci perbedaan)
  4. Jitter histogram / bar perbandingan

Format data yang di-parse:
  [DATA]DELAY,<sample>,<period_us>,<deviation_us>,<drift_us>,<tick>
  [DATA]UNTIL,<sample>,<period_us>,<deviation_us>,<drift_us>,<tick>
  [DATA]STATS,<type>,<mean_us>,<min_us>,<max_us>,<jitter_us>,<total_drift>
  [DATA]COMPARE,<delay_mean>,<until_mean>,<delay_jitter>,<until_jitter>,
                <delay_drift>,<until_drift>
  [DATA]HEAP,<free>,<tick>

Usage:
  python debug_task_delay.py --port /dev/ttyUSB0
  python debug_task_delay.py --file output.log
  python debug_task_delay.py --demo

============================================================================
"""

import sys
import argparse
import time
import re
import math
from collections import defaultdict
from datetime import datetime

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
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


class TaskDelayParser:
    """Parser untuk data serial dari program Task Delay Periodic"""

    def __init__(self):
        # Data vTaskDelay
        self.delay_data = {
            'samples': [],
            'periods': [],
            'deviations': [],
            'drifts': [],
            'ticks': []
        }

        # Data vTaskDelayUntil
        self.until_data = {
            'samples': [],
            'periods': [],
            'deviations': [],
            'drifts': [],
            'ticks': []
        }

        # Statistik ringkasan
        self.stats = {}

        # Data perbandingan
        self.compare_data = {
            'delay_means': [],
            'until_means': [],
            'delay_jitters': [],
            'until_jitters': [],
            'delay_drifts': [],
            'until_drifts': []
        }

        # Heap data
        self.heap_data = {'free': [], 'ticks': []}

        self.raw_lines = []
        self.target_period_us = 100000  # 100ms default

        # Regex patterns
        self.re_delay = re.compile(
            r'\[DATA\]DELAY,(\d+),(\d+),([+-]?\d+),([+-]?\d+),(\d+)'
        )
        self.re_until = re.compile(
            r'\[DATA\]UNTIL,(\d+),(\d+),([+-]?\d+),([+-]?\d+),(\d+)'
        )
        self.re_stats = re.compile(
            r'\[DATA\]STATS,(\w+),(\d+),(\d+),(\d+),(\d+),([+-]?\d+)'
        )
        self.re_compare = re.compile(
            r'\[DATA\]COMPARE,(\d+),(\d+),(\d+),(\d+),([+-]?\d+),([+-]?\d+)'
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

        # Parse DELAY data
        m = self.re_delay.match(line)
        if m:
            self.delay_data['samples'].append(int(m.group(1)))
            self.delay_data['periods'].append(int(m.group(2)))
            self.delay_data['deviations'].append(int(m.group(3)))
            self.delay_data['drifts'].append(int(m.group(4)))
            self.delay_data['ticks'].append(int(m.group(5)))
            return

        # Parse UNTIL data
        m = self.re_until.match(line)
        if m:
            self.until_data['samples'].append(int(m.group(1)))
            self.until_data['periods'].append(int(m.group(2)))
            self.until_data['deviations'].append(int(m.group(3)))
            self.until_data['drifts'].append(int(m.group(4)))
            self.until_data['ticks'].append(int(m.group(5)))
            return

        # Parse STATS
        m = self.re_stats.match(line)
        if m:
            name = m.group(1)
            self.stats[name] = {
                'mean': int(m.group(2)),
                'min': int(m.group(3)),
                'max': int(m.group(4)),
                'jitter': int(m.group(5)),
                'drift': int(m.group(6))
            }
            return

        # Parse COMPARE
        m = self.re_compare.match(line)
        if m:
            self.compare_data['delay_means'].append(int(m.group(1)))
            self.compare_data['until_means'].append(int(m.group(2)))
            self.compare_data['delay_jitters'].append(int(m.group(3)))
            self.compare_data['until_jitters'].append(int(m.group(4)))
            self.compare_data['delay_drifts'].append(int(m.group(5)))
            self.compare_data['until_drifts'].append(int(m.group(6)))
            return

        # Parse HEAP
        m = self.re_heap.match(line)
        if m:
            self.heap_data['free'].append(int(m.group(1)))
            self.heap_data['ticks'].append(int(m.group(2)))
            return

    def get_summary(self):
        """Ringkasan data"""
        summary = []
        summary.append("=" * 65)
        summary.append("  RINGKASAN - vTaskDelay vs vTaskDelayUntil")
        summary.append("=" * 65)

        summary.append(f"\n  Target Period: {self.target_period_us} us ({self.target_period_us/1000:.0f} ms)")

        # vTaskDelay stats
        dd = self.delay_data
        if dd['periods']:
            mean = sum(dd['periods']) / len(dd['periods'])
            dev = mean - self.target_period_us
            summary.append(f"\n  vTaskDelay (Delay RELATIF):")
            summary.append(f"    Sampel      : {len(dd['periods'])}")
            summary.append(f"    Rata-rata   : {mean:.0f} us (deviasi: {dev:+.0f} us)")
            summary.append(f"    Min         : {min(dd['periods'])} us")
            summary.append(f"    Max         : {max(dd['periods'])} us")
            summary.append(f"    Jitter      : {max(dd['periods']) - min(dd['periods'])} us")
            if dd['drifts']:
                summary.append(f"    Total Drift : {dd['drifts'][-1]:+d} us")

        # vTaskDelayUntil stats
        ud = self.until_data
        if ud['periods']:
            mean = sum(ud['periods']) / len(ud['periods'])
            dev = mean - self.target_period_us
            summary.append(f"\n  vTaskDelayUntil (Delay ABSOLUT):")
            summary.append(f"    Sampel      : {len(ud['periods'])}")
            summary.append(f"    Rata-rata   : {mean:.0f} us (deviasi: {dev:+.0f} us)")
            summary.append(f"    Min         : {min(ud['periods'])} us")
            summary.append(f"    Max         : {max(ud['periods'])} us")
            summary.append(f"    Jitter      : {max(ud['periods']) - min(ud['periods'])} us")
            if ud['drifts']:
                summary.append(f"    Total Drift : {ud['drifts'][-1]:+d} us")

        # Conclusion
        if dd['periods'] and ud['periods']:
            d_jitter = max(dd['periods']) - min(dd['periods'])
            u_jitter = max(ud['periods']) - min(ud['periods'])
            summary.append(f"\n  >>> KESIMPULAN <<<")
            if d_jitter > u_jitter:
                summary.append(f"  vTaskDelayUntil LEBIH STABIL")
                summary.append(f"    Jitter: {u_jitter} vs {d_jitter} us ({d_jitter/max(u_jitter,1):.1f}x lebih kecil)")
            if dd['drifts'] and ud['drifts']:
                d_drift = abs(dd['drifts'][-1])
                u_drift = abs(ud['drifts'][-1])
                if d_drift > u_drift:
                    summary.append(f"  vTaskDelay mengakumulasi drift {d_drift - u_drift} us lebih banyak")

        summary.append("=" * 65)
        return "\n".join(summary)


class TaskDelayVisualizer:
    """Visualisasi perbandingan delay"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = None

    def setup_plot(self):
        """Setup figure"""
        self.fig = plt.figure(figsize=(16, 11))
        self.fig.suptitle('STM32 FreeRTOS - vTaskDelay vs vTaskDelayUntil',
                          fontsize=14, fontweight='bold')

        gs = GridSpec(2, 2, figure=self.fig, hspace=0.35, wspace=0.3)

        self.ax_period = self.fig.add_subplot(gs[0, 0])
        self.ax_deviation = self.fig.add_subplot(gs[0, 1])
        self.ax_drift = self.fig.add_subplot(gs[1, 0])
        self.ax_compare = self.fig.add_subplot(gs[1, 1])

    def update_plot(self, frame=None):
        """Update semua subplot"""
        p = self.parser
        target = p.target_period_us

        # --- Period Aktual vs Target ---
        self.ax_period.clear()
        self.ax_period.set_title('Periode Aktual vs Target', fontsize=11)

        if p.delay_data['samples']:
            self.ax_period.plot(p.delay_data['samples'], p.delay_data['periods'],
                                'b-o', markersize=2, label='vTaskDelay',
                                linewidth=1.2, alpha=0.8)

        if p.until_data['samples']:
            self.ax_period.plot(p.until_data['samples'], p.until_data['periods'],
                                'g-s', markersize=2, label='vTaskDelayUntil',
                                linewidth=1.2, alpha=0.8)

        # Target line
        max_sample = max(
            max(p.delay_data['samples']) if p.delay_data['samples'] else 0,
            max(p.until_data['samples']) if p.until_data['samples'] else 0,
            1
        )
        self.ax_period.axhline(y=target, color='red', linestyle='--',
                                linewidth=2, label=f'Target ({target} us)')

        self.ax_period.set_xlabel('Sampel #')
        self.ax_period.set_ylabel('Periode (us)')
        self.ax_period.legend(fontsize=8)
        self.ax_period.grid(True, alpha=0.3)

        # --- Deviation dari Target ---
        self.ax_deviation.clear()
        self.ax_deviation.set_title('Deviasi dari Target (us)', fontsize=11)

        if p.delay_data['samples']:
            self.ax_deviation.scatter(p.delay_data['samples'],
                                       p.delay_data['deviations'],
                                       c='blue', s=10, alpha=0.6,
                                       label='vTaskDelay')

        if p.until_data['samples']:
            self.ax_deviation.scatter(p.until_data['samples'],
                                       p.until_data['deviations'],
                                       c='green', s=10, alpha=0.6,
                                       label='vTaskDelayUntil')

        self.ax_deviation.axhline(y=0, color='red', linestyle='-', linewidth=1)
        self.ax_deviation.axhline(y=500, color='orange', linestyle=':', alpha=0.5,
                                    label='Batas +500us')
        self.ax_deviation.axhline(y=-500, color='orange', linestyle=':', alpha=0.5)

        self.ax_deviation.set_xlabel('Sampel #')
        self.ax_deviation.set_ylabel('Deviasi (us)')
        self.ax_deviation.legend(fontsize=8)
        self.ax_deviation.grid(True, alpha=0.3)

        # --- Drift Kumulatif (GRAFIK KUNCI) ---
        self.ax_drift.clear()
        self.ax_drift.set_title('Drift Kumulatif (Kunci Perbedaan!)', fontsize=11,
                                 color='darkred', fontweight='bold')

        if p.delay_data['samples']:
            self.ax_drift.plot(p.delay_data['samples'], p.delay_data['drifts'],
                                'b-', linewidth=2, label='vTaskDelay (DRIFT!)')
            # Fill area
            self.ax_drift.fill_between(p.delay_data['samples'],
                                        p.delay_data['drifts'],
                                        alpha=0.15, color='blue')

        if p.until_data['samples']:
            self.ax_drift.plot(p.until_data['samples'], p.until_data['drifts'],
                                'g-', linewidth=2, label='vTaskDelayUntil (STABIL)')
            self.ax_drift.fill_between(p.until_data['samples'],
                                        p.until_data['drifts'],
                                        alpha=0.15, color='green')

        self.ax_drift.axhline(y=0, color='red', linestyle='--', linewidth=1)

        self.ax_drift.set_xlabel('Sampel #')
        self.ax_drift.set_ylabel('Drift Kumulatif (us)')
        self.ax_drift.legend(fontsize=9, loc='upper left')
        self.ax_drift.grid(True, alpha=0.3)

        # Annotate drift difference
        if p.delay_data['drifts'] and p.until_data['drifts']:
            d_drift = p.delay_data['drifts'][-1]
            u_drift = p.until_data['drifts'][-1]
            diff = abs(d_drift - u_drift)
            self.ax_drift.annotate(
                f'Selisih: {diff} us\n({diff/1000:.1f} ms)',
                xy=(0.7, 0.85), xycoords='axes fraction',
                fontsize=10, fontweight='bold',
                bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow',
                         alpha=0.8)
            )

        # --- Perbandingan Bar Chart ---
        self.ax_compare.clear()
        self.ax_compare.set_title('Perbandingan Metrik', fontsize=11)

        if p.delay_data['periods'] and p.until_data['periods']:
            metrics = ['Rata-rata\nPeriode (us)', 'Jitter\n(us)', 'Abs Drift\n(us)']

            d_mean = sum(p.delay_data['periods']) / len(p.delay_data['periods'])
            u_mean = sum(p.until_data['periods']) / len(p.until_data['periods'])

            d_jitter = max(p.delay_data['periods']) - min(p.delay_data['periods'])
            u_jitter = max(p.until_data['periods']) - min(p.until_data['periods'])

            d_drift = abs(p.delay_data['drifts'][-1]) if p.delay_data['drifts'] else 0
            u_drift = abs(p.until_data['drifts'][-1]) if p.until_data['drifts'] else 0

            delay_vals = [d_mean, d_jitter, d_drift]
            until_vals = [u_mean, u_jitter, u_drift]

            x = range(len(metrics))
            width = 0.35

            bars1 = self.ax_compare.bar([i - width/2 for i in x], delay_vals,
                                          width, label='vTaskDelay', color='#2196F3',
                                          edgecolor='black')
            bars2 = self.ax_compare.bar([i + width/2 for i in x], until_vals,
                                          width, label='vTaskDelayUntil', color='#4CAF50',
                                          edgecolor='black')

            # Value labels
            for bar in bars1:
                h = bar.get_height()
                self.ax_compare.text(bar.get_x() + bar.get_width()/2., h,
                                      f'{h:.0f}', ha='center', va='bottom',
                                      fontsize=8)
            for bar in bars2:
                h = bar.get_height()
                self.ax_compare.text(bar.get_x() + bar.get_width()/2., h,
                                      f'{h:.0f}', ha='center', va='bottom',
                                      fontsize=8)

            self.ax_compare.set_xticks(list(x))
            self.ax_compare.set_xticklabels(metrics, fontsize=9)
            self.ax_compare.legend(fontsize=8)
            self.ax_compare.grid(axis='y', alpha=0.3)

        self.fig.canvas.draw_idle()

    def show(self):
        """Tampilkan plot"""
        self.setup_plot()
        self.update_plot()
        plt.tight_layout()
        plt.show()


def generate_demo_data(parser):
    """Generate data demo realistis"""
    import random
    random.seed(42)

    print("  Generating demo data for Delay Periodic...")

    target_us = 100000  # 100ms
    work_time_us = 3000  # ~3ms workload

    cum_drift_delay = 0
    cum_drift_until = 0

    for i in range(1, 101):
        tick_delay = i * 103  # vTaskDelay drifts (work + delay)
        tick_until = i * 100  # vTaskDelayUntil stays on target

        # vTaskDelay: period = work + delay + jitter
        # Work time bervariasi sedikit, menyebabkan drift
        work_var = random.randint(-200, 200)
        tick_jitter = random.randint(-50, 50)
        period_delay = target_us + work_time_us + work_var + tick_jitter

        dev_delay = period_delay - target_us
        cum_drift_delay += dev_delay

        if i % 10 == 0:
            parser.parse_line(
                f"[DATA]DELAY,{i},{period_delay},{dev_delay},{cum_drift_delay},{tick_delay * 10}"
            )

        # vTaskDelayUntil: period stays close to target
        # Small jitter from tick resolution only
        period_until = target_us + random.randint(-80, 80)

        dev_until = period_until - target_us
        cum_drift_until += dev_until

        if i % 10 == 0:
            parser.parse_line(
                f"[DATA]UNTIL,{i},{period_until},{dev_until},{cum_drift_until},{tick_until * 10}"
            )

    # Stats
    parser.parse_line(
        f"[DATA]STATS,Task_Delay,{target_us + work_time_us},"
        f"{target_us + work_time_us - 200},{target_us + work_time_us + 200},"
        f"400,{cum_drift_delay}"
    )
    parser.parse_line(
        f"[DATA]STATS,Task_Until,{target_us + 5},"
        f"{target_us - 80},{target_us + 80},"
        f"160,{cum_drift_until}"
    )

    # Compare
    parser.parse_line(
        f"[DATA]COMPARE,{target_us + work_time_us},{target_us + 5},"
        f"400,160,{cum_drift_delay},{cum_drift_until}"
    )

    # Heap
    for i in range(20):
        free = 4800 - random.randint(0, 30)
        parser.parse_line(f"[DATA]HEAP,{free},{i * 5000}")

    print(f"  Generated {len(parser.raw_lines)} data points")
    print(f"  vTaskDelay drift total    : {cum_drift_delay:+d} us")
    print(f"  vTaskDelayUntil drift total: {cum_drift_until:+d} us")


def read_serial(port, baudrate, parser, visualizer=None):
    """Baca dari serial port"""
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
        description='Debug & Visualisasi STM32 FreeRTOS Delay Periodic',
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
    print("  STM32 FreeRTOS - Task Delay Periodic - Debug Tool")
    print("  Perbandingan vTaskDelay vs vTaskDelayUntil")
    print(f"  Waktu: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 65)
    print()

    parser = TaskDelayParser()
    visualizer = None

    if not args.no_plot and HAS_MATPLOTLIB:
        visualizer = TaskDelayVisualizer(parser)

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

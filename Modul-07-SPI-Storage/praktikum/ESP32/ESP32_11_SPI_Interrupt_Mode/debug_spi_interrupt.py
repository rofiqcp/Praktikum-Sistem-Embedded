#!/usr/bin/env python3
"""
Debug Script: SPI Interrupt Mode Performance Analyzer
Modul 07 - SPI & Storage | Program 11

Fitur:
- Parse timing data dari output serial
- Perbandingan blocking vs async vs polling dengan bar charts
- Analisis callback execution
- Timeline visualisasi

Penggunaan:
    python debug_spi_interrupt.py [PORT] [BAUDRATE]
    python debug_spi_interrupt.py /dev/ttyUSB0 115200
"""

import sys
import re
import time
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class SPIInterruptDebugger:
    """Parser dan visualizer untuk SPI interrupt mode."""

    def __init__(self):
        self.mode_timings = {}       # mode -> {'total': us, 'avg': us}
        self.callbacks = {'pre': 0, 'post': 0}
        self.queue_results = []      # List of transaction completion times
        self.current_demo = ""
        self.raw_data = []

    def parse_line(self, line):
        """Parse satu baris output serial."""

        # Track demo context
        if 'Demo 1' in line:
            self.current_demo = 'queued'
        elif 'Demo 2' in line:
            self.current_demo = 'comparison'
        elif 'Demo 3' in line:
            self.current_demo = 'polling'

        # Parse blocking timing
        blocking_match = re.search(
            r'BLOCKING:\s*(\d+)\s*transfers\s*in\s*(\d+)\s*us\s*\(avg:\s*(\d+)', line)
        if blocking_match:
            total_us = int(blocking_match.group(2))
            avg_us = int(blocking_match.group(3))
            self.mode_timings['Blocking'] = {'total': total_us, 'avg': avg_us}
            print(f"  [TIMING] Blocking: total={total_us}us, avg={avg_us}us")

        # Parse non-blocking timing
        nonblocking_match = re.search(
            r'NON-BLOCKING:\s*(\d+)\s*transfers\s*in\s*(\d+)\s*us', line)
        if nonblocking_match:
            total_us = int(nonblocking_match.group(2))
            self.mode_timings['Non-blocking'] = {'total': total_us, 'avg': total_us // 10}
            print(f"  [TIMING] Non-blocking: total={total_us}us")

        # Parse polling timing
        polling_match = re.search(
            r'POLLING:\s*(\d+)\s*transfers\s*in\s*(\d+)\s*us\s*\(avg:\s*(\d+)', line)
        if polling_match:
            total_us = int(polling_match.group(2))
            avg_us = int(polling_match.group(3))
            self.mode_timings['Polling'] = {'total': total_us, 'avg': avg_us}
            print(f"  [TIMING] Polling: total={total_us}us, avg={avg_us}us")

        # Parse queued results
        queued_match = re.search(r'Queued.*?in\s*(\d+)\s*us', line)
        if queued_match and 'queue' in line.lower():
            queue_us = int(queued_match.group(1))
            self.mode_timings.setdefault('Queued', {})['total'] = queue_us

        # Parse callback counts
        cb_match = re.search(r'Pre:\s*(\d+),?\s*Post:\s*(\d+)', line)
        if cb_match:
            self.callbacks['pre'] = int(cb_match.group(1))
            self.callbacks['post'] = int(cb_match.group(2))
            print(f"  [CALLBACKS] Pre: {self.callbacks['pre']}, Post: {self.callbacks['post']}")

        # Parse speedup
        speedup_match = re.search(r'Speedup:\s*([\d.]+)x', line)
        if speedup_match:
            print(f"  [SPEEDUP] {speedup_match.group(1)}x")

        # Parse fastest mode
        fastest_match = re.search(r'Fastest mode:\s*(\w+)\s*\((\d+)', line)
        if fastest_match:
            print(f"  [FASTEST] {fastest_match.group(1)}: {fastest_match.group(2)}us")

        self.raw_data.append(line)

    def print_summary(self):
        """Print ringkasan perbandingan mode."""
        print("\n" + "=" * 60)
        print("        SPI Transfer Mode Comparison")
        print("=" * 60)

        if not self.mode_timings:
            print("  No timing data captured.")
            return

        print(f"\n  {'Mode':<15s} {'Total(us)':>12s} {'Avg(us)':>10s}")
        print(f"  {'-'*15} {'-'*12} {'-'*10}")
        for mode, data in sorted(self.mode_timings.items()):
            total = data.get('total', 0)
            avg = data.get('avg', 0)
            print(f"  {mode:<15s} {total:>12d} {avg:>10d}")

        print(f"\n  Callbacks: Pre={self.callbacks['pre']}, Post={self.callbacks['post']}")
        print("=" * 60)

    def plot_results(self):
        """Generate visualisasi perbandingan."""
        if not HAS_MATPLOTLIB or not self.mode_timings:
            print("[WARN] Cannot plot: matplotlib or data missing")
            return

        fig, axes = plt.subplots(1, 3, figsize=(16, 6))
        fig.suptitle('SPI Transfer Mode Comparison', fontsize=14, fontweight='bold')

        modes = list(self.mode_timings.keys())
        colors = {'Blocking': '#3498db', 'Non-blocking': '#2ecc71',
                  'Polling': '#e74c3c', 'Queued': '#f39c12'}

        # Plot 1: Total time comparison
        ax1 = axes[0]
        totals = [self.mode_timings[m].get('total', 0) for m in modes]
        bars = ax1.bar(modes, totals,
                       color=[colors.get(m, '#95a5a6') for m in modes],
                       edgecolor='black')
        for bar, val in zip(bars, totals):
            ax1.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + max(totals) * 0.02,
                     f'{val}us', ha='center', va='bottom', fontweight='bold', fontsize=9)
        ax1.set_title('Total Transfer Time')
        ax1.set_ylabel('Time (us)')
        ax1.grid(True, alpha=0.3, axis='y')

        # Plot 2: Average per-transfer time
        ax2 = axes[1]
        avgs = [self.mode_timings[m].get('avg', 0) for m in modes]
        bars = ax2.bar(modes, avgs,
                       color=[colors.get(m, '#95a5a6') for m in modes],
                       edgecolor='black')
        for bar, val in zip(bars, avgs):
            ax2.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + max(avgs) * 0.02,
                     f'{val}us', ha='center', va='bottom', fontweight='bold', fontsize=9)
        ax2.set_title('Average Per-Transfer Time')
        ax2.set_ylabel('Time (us)')
        ax2.grid(True, alpha=0.3, axis='y')

        # Plot 3: Relative performance (normalized to slowest)
        ax3 = axes[2]
        if totals and max(totals) > 0:
            max_t = max(totals)
            normalized = [(max_t / t * 100 if t > 0 else 0) for t in totals]
            bars = ax3.barh(modes, normalized,
                            color=[colors.get(m, '#95a5a6') for m in modes],
                            edgecolor='black')
            for bar, val in zip(bars, normalized):
                ax3.text(bar.get_width() + 1, bar.get_y() + bar.get_height() / 2.,
                         f'{val:.0f}%', ha='left', va='center', fontweight='bold')
            ax3.set_xlabel('Relative Speed (%)')
            ax3.set_title('Relative Performance\n(100% = baseline)')
            ax3.axvline(x=100, color='gray', linestyle='--', alpha=0.5)
        ax3.grid(True, alpha=0.3, axis='x')

        plt.tight_layout()
        plt.savefig('spi_interrupt_analysis.png', dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to spi_interrupt_analysis.png")
        plt.show()


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    debugger = SPIInterruptDebugger()

    if not HAS_SERIAL:
        print("[INFO] Running in demo mode with sample data...")
        demo_lines = [
            "Demo 1: Queued (Non-blocking) Transfers",
            "Queued 10/10 transactions in 45 us",
            "All 10 transactions completed in 1250 us",
            "Callbacks - Pre: 10, Post: 10",
            "Demo 2: Blocking vs Non-blocking",
            "BLOCKING: 10 transfers in 1580 us (avg: 158 us/transfer)",
            "NON-BLOCKING: 10 transfers in 1320 us (queue: 52 us, total: 1320 us)",
            "Speedup: 1.20x (non-blocking faster)",
            "Demo 3: Polling Mode Transfer",
            "POLLING: 10 transfers in 980 us (avg: 98 us/transfer)",
            "Fastest mode: Polling (980 us)",
            "Pre-transfer callbacks:  30",
            "Post-transfer callbacks: 30",
        ]
        for line in demo_lines:
            debugger.parse_line(line)
            time.sleep(0.05)

        debugger.print_summary()
        debugger.plot_results()
        return

    print(f"[INFO] Connecting to {port} at {baud} baud...")
    print("[INFO] Press Ctrl+C to stop and show results\n")

    try:
        ser = serial.Serial(port, baud, timeout=1)
        while True:
            if ser.in_waiting > 0:
                raw = ser.readline()
                try:
                    line = raw.decode('utf-8', errors='replace').strip()
                except Exception:
                    continue
                if line:
                    print(f"[SERIAL] {line}")
                    debugger.parse_line(line)
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    finally:
        debugger.print_summary()
        debugger.plot_results()


if __name__ == '__main__':
    main()

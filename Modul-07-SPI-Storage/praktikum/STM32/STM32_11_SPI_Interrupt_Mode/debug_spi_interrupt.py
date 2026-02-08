#!/usr/bin/env python3
"""
Debug SPI Interrupt Mode - STM32_11_SPI_Interrupt_Mode
Compare blocking vs interrupt timing, CPU utilization chart.

Usage:
    python debug_spi_interrupt.py /dev/ttyUSB0 115200
    python debug_spi_interrupt.py log_file.txt
"""

import sys
import re
import os
import time

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install with: pip install matplotlib")


class SPIInterruptDebugger:
    """Parse and visualize SPI interrupt mode comparison results."""

    def __init__(self):
        self.raw_lines = []
        self.csv_data = []
        self.csv_header = None
        self.in_csv = False
        self.blocking_times = []
        self.interrupt_times = []
        self.txrx_times = []
        self.blocking_ops = []
        self.interrupt_ops = []
        self.cpu_baseline = 0
        self.cpu_blocking = 0
        self.cpu_interrupt_ops = 0
        self.cpu_availability = 0

    def parse_line(self, line):
        """Parse a single log line."""
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # CSV
        if line == "[CSV_START]":
            self.in_csv = True
            self.csv_header = None
            return
        if line == "[CSV_END]":
            self.in_csv = False
            return
        if self.in_csv:
            if self.csv_header is None:
                self.csv_header = line.split(',')
            else:
                values = line.split(',')
                if len(values) == len(self.csv_header):
                    row = {}
                    for h, v in zip(self.csv_header, values):
                        try:
                            row[h] = int(v)
                        except ValueError:
                            row[h] = v
                    self.csv_data.append(row)
            return

        # Blocking results
        m = re.match(r"\[BLOCK\] Iter\s+(\d+):\s+(\d+) us \(CPU ops: (\d+)\)", line)
        if m:
            self.blocking_times.append(int(m.group(2)))
            self.blocking_ops.append(int(m.group(3)))
            return

        # Interrupt TX results
        m = re.match(r"\[INT\] Iter\s+(\d+):\s+(\d+) us \(CPU ops: (\d+)\)", line)
        if m:
            self.interrupt_times.append(int(m.group(2)))
            self.interrupt_ops.append(int(m.group(3)))
            return

        # TXRX results
        m = re.match(r"\[TXRX\] Iter\s+(\d+):\s+(\d+) us", line)
        if m:
            self.txrx_times.append(int(m.group(2)))
            return

        # CPU utilization
        m = re.search(r"Baseline ops \(no SPI\): (\d+)", line)
        if m:
            self.cpu_baseline = int(m.group(1))
        m = re.search(r"CPU availability: ~(\d+)%", line)
        if m:
            self.cpu_availability = int(m.group(1))

    def parse_stream(self, port, baudrate=115200):
        """Read from serial port."""
        if not HAS_SERIAL:
            print("[ERROR] pyserial not installed.")
            return
        print(f"[INFO] Connecting to {port} at {baudrate} baud...")
        ser = serial.Serial(port, baudrate, timeout=1)
        print("[INFO] Connected. Press Ctrl+C to stop.\n")
        try:
            while True:
                line = ser.readline().decode('utf-8', errors='replace')
                if line:
                    print(line, end='')
                    self.parse_line(line)
        except KeyboardInterrupt:
            print("\n[INFO] Stopped.")
        finally:
            ser.close()

    def parse_file(self, filename):
        """Parse a saved log file."""
        with open(filename, 'r') as f:
            for line in f:
                self.parse_line(line)
        print(f"[INFO] Parsed {len(self.raw_lines)} lines")

    def print_summary(self):
        """Print text summary."""
        print("\n" + "=" * 60)
        print("  SPI Interrupt Mode Debug Summary")
        print("=" * 60)

        if self.blocking_times:
            avg_b = sum(self.blocking_times) / len(self.blocking_times)
            print(f"\n  Blocking TX  : avg {avg_b:.1f} us ({len(self.blocking_times)} samples)")
        if self.interrupt_times:
            avg_i = sum(self.interrupt_times) / len(self.interrupt_times)
            print(f"  Interrupt TX : avg {avg_i:.1f} us ({len(self.interrupt_times)} samples)")
        if self.txrx_times:
            avg_t = sum(self.txrx_times) / len(self.txrx_times)
            print(f"  Interrupt TXRX: avg {avg_t:.1f} us ({len(self.txrx_times)} samples)")

        if self.interrupt_ops:
            avg_ops = sum(self.interrupt_ops) / len(self.interrupt_ops)
            print(f"\n  Avg CPU ops during interrupt TX: {avg_ops:.0f}")
        print(f"  CPU availability: {self.cpu_availability}%")

        if self.blocking_times and self.interrupt_times:
            avg_b = sum(self.blocking_times) / len(self.blocking_times)
            avg_i = sum(self.interrupt_times) / len(self.interrupt_times)
            overhead = ((avg_i - avg_b) / avg_b) * 100 if avg_b > 0 else 0
            print(f"  Interrupt overhead: {overhead:+.1f}%")

    def plot(self):
        """Generate visualization plots."""
        if not HAS_MATPLOTLIB:
            print("[WARN] Cannot plot without matplotlib")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle("SPI Blocking vs Interrupt Mode Analysis", fontsize=14, fontweight='bold')

        # 1. Timing comparison (bar chart)
        ax1 = axes[0, 0]
        if self.blocking_times and self.interrupt_times:
            x = np.arange(min(len(self.blocking_times), len(self.interrupt_times)))
            bt = self.blocking_times[:len(x)]
            it = self.interrupt_times[:len(x)]
            width = 0.35
            ax1.bar(x - width/2, bt, width, label='Blocking', color='#F44336', alpha=0.8)
            ax1.bar(x + width/2, it, width, label='Interrupt', color='#4CAF50', alpha=0.8)
            ax1.set_xlabel("Iteration")
            ax1.set_ylabel("Time (µs)")
            ax1.set_title("Transfer Time: Blocking vs Interrupt")
            ax1.legend()
            ax1.grid(True, alpha=0.3, axis='y')

        # 2. CPU ops comparison
        ax2 = axes[0, 1]
        if self.blocking_ops and self.interrupt_ops:
            x = np.arange(min(len(self.blocking_ops), len(self.interrupt_ops)))
            bo = self.blocking_ops[:len(x)]
            io = self.interrupt_ops[:len(x)]
            ax2.bar(x - 0.2, bo, 0.4, label='Blocking (CPU stuck)', color='#F44336', alpha=0.8)
            ax2.bar(x + 0.2, io, 0.4, label='Interrupt (CPU free)', color='#4CAF50', alpha=0.8)
            ax2.set_xlabel("Iteration")
            ax2.set_ylabel("CPU Operations")
            ax2.set_title("CPU Work During Transfer")
            ax2.legend()
            ax2.grid(True, alpha=0.3, axis='y')

        # 3. CPU utilization pie chart
        ax3 = axes[1, 0]
        if self.cpu_availability > 0:
            sizes = [self.cpu_availability, 100 - self.cpu_availability]
            labels = [f'CPU Available\n({self.cpu_availability}%)',
                      f'SPI Overhead\n({100-self.cpu_availability}%)']
            colors = ['#4CAF50', '#FF9800']
            explode = (0.05, 0)
            ax3.pie(sizes, explode=explode, labels=labels, colors=colors,
                    autopct='%1.1f%%', startangle=90, textprops={'fontsize': 9})
            ax3.set_title("CPU Availability in Interrupt Mode")
        else:
            # Estimate from ops data
            if self.blocking_ops and self.interrupt_ops:
                avg_bo = sum(self.blocking_ops) / len(self.blocking_ops)
                avg_io = sum(self.interrupt_ops) / len(self.interrupt_ops)
                ax3.bar(['Blocking', 'Interrupt'], [avg_bo, avg_io],
                        color=['#F44336', '#4CAF50'])
                ax3.set_ylabel("Avg CPU Operations")
                ax3.set_title("CPU Ops During Transfer")

        # 4. All modes comparison
        ax4 = axes[1, 1]
        modes = []
        avgs = []
        colors = []
        if self.blocking_times:
            modes.append('Blocking TX')
            avgs.append(sum(self.blocking_times) / len(self.blocking_times))
            colors.append('#F44336')
        if self.interrupt_times:
            modes.append('Interrupt TX')
            avgs.append(sum(self.interrupt_times) / len(self.interrupt_times))
            colors.append('#4CAF50')
        if self.txrx_times:
            modes.append('Interrupt TXRX')
            avgs.append(sum(self.txrx_times) / len(self.txrx_times))
            colors.append('#2196F3')

        if modes:
            bars = ax4.bar(modes, avgs, color=colors, alpha=0.8, edgecolor='#333')
            ax4.set_ylabel("Average Time (µs)")
            ax4.set_title("Transfer Mode Comparison")
            ax4.grid(True, alpha=0.3, axis='y')
            for bar, val in zip(bars, avgs):
                ax4.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                         f"{val:.1f}µs", ha='center', va='bottom', fontsize=9)

        plt.tight_layout()
        plt.savefig("spi_interrupt_debug.png", dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to spi_interrupt_debug.png")
        plt.show()


def generate_sample_data():
    """Generate sample data for testing."""
    import random
    lines = []
    # Blocking
    for i in range(10):
        t = 115 + random.randint(-5, 5)
        lines.append(f"[BLOCK] Iter  {i}: {t} us (CPU ops: 0)")
    # Interrupt
    for i in range(10):
        t = 120 + random.randint(-8, 8)
        ops = 45 + random.randint(-10, 15)
        lines.append(f"[INT] Iter  {i}: {t} us (CPU ops: {ops})")
    # TXRX
    for i in range(10):
        t = 125 + random.randint(-6, 6)
        lines.append(f"[TXRX] Iter  {i}: {t} us, CPU ops: 40, data MATCH")
    # CPU
    lines.append("[CPU] Baseline ops (no SPI): 50000 in ~10ms")
    lines.append("[CPU] CPU availability: ~38%")
    # CSV
    lines.append("[CSV_START]")
    lines.append("iter,blocking_us,interrupt_us,txrx_us,blocking_ops,interrupt_ops")
    for i in range(10):
        bt = 115 + random.randint(-5, 5)
        it = 120 + random.randint(-8, 8)
        tt = 125 + random.randint(-6, 6)
        io = 45 + random.randint(-10, 15)
        lines.append(f"{i},{bt},{it},{tt},0,{io}")
    lines.append("[CSV_END]")
    return "\n".join(lines)


if __name__ == "__main__":
    debugger = SPIInterruptDebugger()

    if len(sys.argv) >= 2:
        port_or_file = sys.argv[1]
        if os.path.isfile(port_or_file):
            debugger.parse_file(port_or_file)
        elif HAS_SERIAL:
            baudrate = int(sys.argv[2]) if len(sys.argv) >= 3 else 115200
            debugger.parse_stream(port_or_file, baudrate)
        else:
            print(f"[ERROR] Cannot open {port_or_file}")
            sys.exit(1)
    else:
        print("[INFO] No input. Using sample data for demo.")
        for line in generate_sample_data().split('\n'):
            debugger.parse_line(line)

    debugger.print_summary()

    if HAS_MATPLOTLIB:
        debugger.plot()

#!/usr/bin/env python3
"""
Debug SPI Speed Benchmark - STM32_10_SPI_Speed_Benchmark
Parse benchmark results, create heatmap, efficiency chart, and prescaler comparison.

Usage:
    python debug_spi_benchmark.py /dev/ttyUSB0 115200
    python debug_spi_benchmark.py log_file.txt
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
    import matplotlib.colors as mcolors
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install with: pip install matplotlib")


class SPIBenchmarkDebugger:
    """Parse and visualize SPI benchmark results."""

    def __init__(self):
        self.raw_lines = []
        self.csv_data = []  # list of dicts
        self.in_csv = False
        self.csv_header = None
        self.prescalers = []
        self.buffer_sizes = []
        self.best_throughput = 0
        self.best_prescaler = 0
        self.best_bufsize = 0

    def parse_line(self, line):
        """Parse a single log line."""
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # CSV data extraction
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
                            try:
                                row[h] = float(v)
                            except ValueError:
                                row[h] = v
                    self.csv_data.append(row)
            return

        # Best result
        m = re.search(r"Best throughput\s*:\s*(\d+) bytes/sec", line)
        if m:
            self.best_throughput = int(m.group(1))
        m = re.search(r"Best prescaler\s*:\s*/(\d+)", line)
        if m:
            self.best_prescaler = int(m.group(1))
        m = re.search(r"Best buffer size\s*:\s*(\d+)", line)
        if m:
            self.best_bufsize = int(m.group(1))

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
        print(f"[INFO] Parsed {len(self.csv_data)} benchmark data points")

    def print_summary(self):
        """Print text summary."""
        print("\n" + "=" * 60)
        print("  SPI Speed Benchmark Summary")
        print("=" * 60)
        print(f"  Data points      : {len(self.csv_data)}")
        print(f"  Best throughput   : {self.best_throughput:,} bytes/sec")
        print(f"  Best prescaler    : /{self.best_prescaler}")
        print(f"  Best buffer size  : {self.best_bufsize} bytes")

        if self.csv_data:
            prescalers = sorted(set(d.get('prescaler_div', 0) for d in self.csv_data))
            bufsizes = sorted(set(d.get('buffer_size', 0) for d in self.csv_data))
            print(f"\n  Prescalers tested : {prescalers}")
            print(f"  Buffer sizes      : {bufsizes}")

    def plot(self):
        """Generate visualization plots."""
        if not HAS_MATPLOTLIB or not self.csv_data:
            print("[WARN] Cannot plot (no matplotlib or no data)")
            return

        prescalers = sorted(set(d['prescaler_div'] for d in self.csv_data))
        bufsizes = sorted(set(d['buffer_size'] for d in self.csv_data))

        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle("SPI Speed Benchmark Analysis", fontsize=14, fontweight='bold')

        # 1. Throughput Heatmap
        ax1 = axes[0, 0]
        heatmap = np.zeros((len(prescalers), len(bufsizes)))
        for d in self.csv_data:
            pi = prescalers.index(d['prescaler_div'])
            bi = bufsizes.index(d['buffer_size'])
            heatmap[pi, bi] = d['throughput_bps'] / 1024  # KB/s

        im = ax1.imshow(heatmap, cmap='YlOrRd', aspect='auto', interpolation='nearest')
        ax1.set_xticks(range(len(bufsizes)))
        ax1.set_xticklabels([str(b) for b in bufsizes], fontsize=8)
        ax1.set_yticks(range(len(prescalers)))
        ax1.set_yticklabels([f"/{p}" for p in prescalers], fontsize=8)
        ax1.set_xlabel("Buffer Size (bytes)")
        ax1.set_ylabel("Prescaler")
        ax1.set_title("Throughput Heatmap (KB/s)")
        cbar = plt.colorbar(im, ax=ax1, shrink=0.8)
        cbar.set_label("KB/s")

        # Add text annotations
        for i in range(len(prescalers)):
            for j in range(len(bufsizes)):
                ax1.text(j, i, f"{heatmap[i,j]:.0f}",
                         ha='center', va='center', fontsize=6,
                         color='white' if heatmap[i,j] > heatmap.max()*0.6 else 'black')

        # 2. Efficiency Chart
        ax2 = axes[0, 1]
        eff_map = np.zeros((len(prescalers), len(bufsizes)))
        for d in self.csv_data:
            pi = prescalers.index(d['prescaler_div'])
            bi = bufsizes.index(d['buffer_size'])
            eff_map[pi, bi] = d.get('efficiency_pct', 0)

        im2 = ax2.imshow(eff_map, cmap='RdYlGn', aspect='auto',
                         interpolation='nearest', vmin=0, vmax=100)
        ax2.set_xticks(range(len(bufsizes)))
        ax2.set_xticklabels([str(b) for b in bufsizes], fontsize=8)
        ax2.set_yticks(range(len(prescalers)))
        ax2.set_yticklabels([f"/{p}" for p in prescalers], fontsize=8)
        ax2.set_xlabel("Buffer Size (bytes)")
        ax2.set_ylabel("Prescaler")
        ax2.set_title("Transfer Efficiency (%)")
        cbar2 = plt.colorbar(im2, ax=ax2, shrink=0.8)
        cbar2.set_label("%")

        for i in range(len(prescalers)):
            for j in range(len(bufsizes)):
                ax2.text(j, i, f"{eff_map[i,j]:.0f}%",
                         ha='center', va='center', fontsize=6)

        # 3. Throughput vs Buffer Size (line chart per prescaler)
        ax3 = axes[1, 0]
        colors = plt.cm.viridis(np.linspace(0, 1, len(prescalers)))
        for pi, pval in enumerate(prescalers):
            tp_data = []
            for bs in bufsizes:
                for d in self.csv_data:
                    if d['prescaler_div'] == pval and d['buffer_size'] == bs:
                        tp_data.append(d['throughput_bps'] / 1024)
                        break
                else:
                    tp_data.append(0)
            ax3.plot(bufsizes, tp_data, 'o-', color=colors[pi],
                     label=f"/{pval}", linewidth=1.5, markersize=4)

        ax3.set_xlabel("Buffer Size (bytes)")
        ax3.set_ylabel("Throughput (KB/s)")
        ax3.set_title("Throughput vs Buffer Size")
        ax3.legend(title="Prescaler", fontsize=7, ncol=2)
        ax3.set_xscale('log', base=2)
        ax3.grid(True, alpha=0.3)

        # 4. Max throughput per prescaler (bar chart)
        ax4 = axes[1, 1]
        max_tp = []
        for pval in prescalers:
            tp_vals = [d['throughput_bps'] for d in self.csv_data
                       if d['prescaler_div'] == pval]
            max_tp.append(max(tp_vals) / 1024 if tp_vals else 0)

        # Theoretical max
        theoretical = [72000000 / p / 8 / 1024 for p in prescalers]

        x = np.arange(len(prescalers))
        width = 0.35
        ax4.bar(x - width/2, max_tp, width, label='Actual', color='#2196F3')
        ax4.bar(x + width/2, theoretical, width, label='Theoretical', color='#E0E0E0',
                edgecolor='#999')
        ax4.set_xticks(x)
        ax4.set_xticklabels([f"/{p}" for p in prescalers], fontsize=8)
        ax4.set_xlabel("Prescaler")
        ax4.set_ylabel("Throughput (KB/s)")
        ax4.set_title("Max Throughput: Actual vs Theoretical")
        ax4.legend()
        ax4.grid(True, alpha=0.3, axis='y')

        plt.tight_layout()
        plt.savefig("spi_benchmark_debug.png", dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to spi_benchmark_debug.png")
        plt.show()


def generate_sample_data():
    """Generate sample CSV data for testing."""
    lines = ["[CSV_START]",
             "prescaler_div,spi_clock_hz,buffer_size,time_us,throughput_bps,efficiency_pct"]
    prescalers = [(2, 36000000), (4, 18000000), (8, 9000000), (16, 4500000),
                  (32, 2250000), (64, 1125000), (128, 562500), (256, 281250)]
    bufsizes = [8, 16, 32, 64, 128, 256, 512]

    for div, clk in prescalers:
        for bs in bufsizes:
            theoretical = clk / 8
            # Simulate ~60-90% efficiency depending on buffer size
            eff = 0.55 + 0.35 * (bs / 512)
            tp = int(theoretical * eff)
            total_bytes = bs * 100
            time_us = int(total_bytes * 1000000 / tp) if tp > 0 else 1
            eff_pct = int(eff * 100)
            lines.append(f"{div},{clk},{bs},{time_us},{tp},{eff_pct}")

    lines.append("[CSV_END]")
    lines.append("Best throughput     : 3240000 bytes/sec (3164 KB/s)")
    lines.append("Best prescaler      : /2")
    lines.append("Best buffer size    : 512 bytes")
    return "\n".join(lines)


if __name__ == "__main__":
    debugger = SPIBenchmarkDebugger()

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

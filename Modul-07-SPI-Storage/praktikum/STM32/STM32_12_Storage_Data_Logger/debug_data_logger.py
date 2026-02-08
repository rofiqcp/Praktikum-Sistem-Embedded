#!/usr/bin/env python3
"""
Debug Data Logger - STM32_12_Storage_Data_Logger
Parse CSV data, plot time series, storage gauge, detect anomalies.

Usage:
    python debug_data_logger.py /dev/ttyUSB0 115200
    python debug_data_logger.py log_file.txt
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
    import matplotlib.gridspec as gridspec
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install with: pip install matplotlib")


class DataLoggerDebugger:
    """Parse and visualize data logger output."""

    def __init__(self):
        self.raw_lines = []
        self.entries = []  # list of dicts: timestamp_ms, temperature_c, humidity_pct, adc_raw
        self.in_csv = False
        self.in_csv_all = False
        self.csv_header = None
        self.stats = {}
        self.page_info = []  # list of (page, addr, entries, max, status)
        self.total_entries = 0
        self.flash_usage_pct = 0
        self.page_erases = 0
        self.wrapped = False
        self.anomalies = []

    def parse_line(self, line):
        """Parse a single log line."""
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # CSV data (last N entries)
        if line == "[CSV_START]":
            self.in_csv = True
            self.csv_header = None
            return
        if line == "[CSV_END]":
            self.in_csv = False
            return

        # CSV all entries
        if line == "[CSV_ALL_START]":
            self.in_csv_all = True
            self.csv_header = None
            return
        if line == "[CSV_ALL_END]":
            self.in_csv_all = False
            return

        if self.in_csv or self.in_csv_all:
            if self.csv_header is None:
                self.csv_header = line.split(',')
            else:
                values = line.split(',')
                if len(values) == len(self.csv_header):
                    row = {}
                    for h, v in zip(self.csv_header, values):
                        try:
                            row[h] = float(v)
                        except ValueError:
                            row[h] = v
                    if self.in_csv_all:
                        self.entries.append(row)
                    elif self.in_csv:
                        self.entries.append(row)
            return

        # Statistics parsing
        m = re.search(r"Total entries\s*:\s*(\d+)", line)
        if m:
            self.total_entries = int(m.group(1))

        m = re.search(r"Flash usage\s*:\s*\d+ / \d+ bytes \((\d+)%\)", line)
        if m:
            self.flash_usage_pct = int(m.group(1))

        m = re.search(r"Page erases\s*:\s*(\d+)", line)
        if m:
            self.page_erases = int(m.group(1))

        m = re.search(r"Wrapped\s*:\s*(YES|NO)", line)
        if m:
            self.wrapped = (m.group(1) == "YES")

        # Page usage table
        m = re.match(r"\s*(\d+)\s*│\s*0x([0-9A-Fa-f]+)\s*│\s*(\d+)/\s*(\d+)\s*│\s*(\w+)", line)
        if m:
            self.page_info.append({
                'page': int(m.group(1)),
                'addr': int(m.group(2), 16),
                'entries': int(m.group(3)),
                'max': int(m.group(4)),
                'status': m.group(5)
            })

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
        print(f"[INFO] Parsed {len(self.entries)} data entries from {filename}")

    def detect_anomalies(self):
        """Detect anomalies in sensor data."""
        self.anomalies = []
        if len(self.entries) < 3:
            return

        temps = [e.get('temperature_c', 0) for e in self.entries]
        humids = [e.get('humidity_pct', 0) for e in self.entries]

        # Check for sudden temperature jumps (>5°C)
        for i in range(1, len(temps)):
            diff = abs(temps[i] - temps[i-1])
            if diff > 5.0:
                self.anomalies.append({
                    'index': i,
                    'type': 'TEMP_JUMP',
                    'message': f"Temperature jump: {temps[i-1]:.1f} -> {temps[i]:.1f}°C (Δ={diff:.1f}°C)"
                })

        # Check for out-of-range values
        for i, e in enumerate(self.entries):
            t = e.get('temperature_c', 25)
            h = e.get('humidity_pct', 65)
            if t < -40 or t > 85:
                self.anomalies.append({
                    'index': i, 'type': 'TEMP_RANGE',
                    'message': f"Temperature out of range: {t:.1f}°C"
                })
            if h < 0 or h > 100:
                self.anomalies.append({
                    'index': i, 'type': 'HUMID_RANGE',
                    'message': f"Humidity out of range: {h:.1f}%"
                })

        # Check for timestamp gaps
        timestamps = [e.get('timestamp_ms', 0) for e in self.entries]
        for i in range(1, len(timestamps)):
            gap = timestamps[i] - timestamps[i-1]
            if gap > 2000 and gap > 0:  # More than 2x expected interval
                self.anomalies.append({
                    'index': i, 'type': 'TIME_GAP',
                    'message': f"Timestamp gap: {gap:.0f}ms (expected ~500ms)"
                })

    def print_summary(self):
        """Print text summary."""
        print("\n" + "=" * 60)
        print("  Data Logger Debug Summary")
        print("=" * 60)
        print(f"  Total entries     : {len(self.entries)}")
        print(f"  Flash entries     : {self.total_entries}")
        print(f"  Flash usage       : {self.flash_usage_pct}%")
        print(f"  Page erases       : {self.page_erases}")
        print(f"  Wrapped           : {'Yes' if self.wrapped else 'No'}")

        if self.entries:
            temps = [e.get('temperature_c', 0) for e in self.entries]
            humids = [e.get('humidity_pct', 0) for e in self.entries]
            adcs = [e.get('adc_raw', 0) for e in self.entries]

            print(f"\n  Temperature range : {min(temps):.1f}°C - {max(temps):.1f}°C")
            print(f"  Humidity range    : {min(humids):.1f}% - {max(humids):.1f}%")
            print(f"  ADC range         : {min(adcs):.0f} - {max(adcs):.0f}")

            ts = [e.get('timestamp_ms', 0) for e in self.entries]
            if len(ts) > 1:
                duration = (ts[-1] - ts[0]) / 1000
                print(f"  Duration          : {duration:.1f} sec")

        self.detect_anomalies()
        if self.anomalies:
            print(f"\n  Anomalies detected: {len(self.anomalies)}")
            for a in self.anomalies[:10]:
                print(f"    [{a['type']}] idx={a['index']}: {a['message']}")

        if self.page_info:
            print(f"\n  Page Status:")
            for p in self.page_info:
                bar = '█' * p['entries'] + '░' * (p['max'] - p['entries'])
                print(f"    Page {p['page']}: [{bar}] {p['entries']}/{p['max']} {p['status']}")

    def plot(self):
        """Generate visualization plots."""
        if not HAS_MATPLOTLIB or not self.entries:
            print("[WARN] Cannot plot (no matplotlib or no data)")
            return

        fig = plt.figure(figsize=(16, 12))
        fig.suptitle("Data Logger Analysis", fontsize=14, fontweight='bold')
        gs = gridspec.GridSpec(3, 3, figure=fig, hspace=0.4, wspace=0.35)

        timestamps = [e.get('timestamp_ms', 0) for e in self.entries]
        temps = [e.get('temperature_c', 25) for e in self.entries]
        humids = [e.get('humidity_pct', 65) for e in self.entries]
        adcs = [e.get('adc_raw', 0) for e in self.entries]

        # Convert timestamps to seconds
        t0 = timestamps[0] if timestamps else 0
        t_sec = [(t - t0) / 1000.0 for t in timestamps]

        # 1. Temperature time series
        ax1 = fig.add_subplot(gs[0, :])
        ax1.plot(t_sec, temps, '-', color='#F44336', linewidth=0.8, label='Temperature')
        ax1.set_xlabel("Time (sec)")
        ax1.set_ylabel("Temperature (°C)")
        ax1.set_title("Temperature Over Time")
        ax1.grid(True, alpha=0.3)
        ax1.legend()

        # Mark anomalies
        temp_anomalies = [a for a in self.anomalies if 'TEMP' in a['type']]
        for a in temp_anomalies:
            idx = a['index']
            if idx < len(t_sec):
                ax1.axvline(x=t_sec[idx], color='red', linestyle='--', alpha=0.5)

        # 2. Humidity time series
        ax2 = fig.add_subplot(gs[1, :2])
        ax2.plot(t_sec, humids, '-', color='#2196F3', linewidth=0.8, label='Humidity')
        ax2.set_xlabel("Time (sec)")
        ax2.set_ylabel("Humidity (%)")
        ax2.set_title("Humidity Over Time")
        ax2.grid(True, alpha=0.3)
        ax2.legend()

        # 3. Storage gauge
        ax3 = fig.add_subplot(gs[1, 2])
        usage = self.flash_usage_pct if self.flash_usage_pct > 0 else \
                (len(self.entries) * 100 // 680 if self.entries else 0)
        colors_gauge = ['#4CAF50' if usage < 70 else '#FF9800' if usage < 90 else '#F44336']
        ax3.barh([0], [usage], color=colors_gauge, height=0.5, edgecolor='#333')
        ax3.barh([0], [100], color='#E0E0E0', height=0.5, edgecolor='#333', zorder=0)
        ax3.set_xlim(0, 110)
        ax3.set_yticks([])
        ax3.set_xlabel("Usage %")
        ax3.set_title(f"Flash Storage Usage ({usage}%)")
        ax3.text(usage/2, 0, f"{usage}%", ha='center', va='center',
                 fontsize=14, fontweight='bold', color='white')

        # 4. ADC sawtooth
        ax4 = fig.add_subplot(gs[2, 0])
        ax4.plot(t_sec, adcs, '-', color='#FF9800', linewidth=0.8)
        ax4.set_xlabel("Time (sec)")
        ax4.set_ylabel("ADC Raw")
        ax4.set_title("ADC Sawtooth Wave")
        ax4.grid(True, alpha=0.3)

        # 5. Page usage bar chart
        ax5 = fig.add_subplot(gs[2, 1])
        if self.page_info:
            pages = [p['page'] for p in self.page_info]
            entries_ct = [p['entries'] for p in self.page_info]
            max_entries = self.page_info[0]['max'] if self.page_info else 85
            status_colors = {
                'FULL': '#4CAF50', 'WRITING': '#2196F3',
                'EMPTY': '#E0E0E0', 'OLDEST': '#FF9800', 'PARTIAL': '#FFC107'
            }
            colors_bar = [status_colors.get(p['status'], '#999') for p in self.page_info]
            ax5.bar(pages, entries_ct, color=colors_bar, edgecolor='#333', linewidth=0.5)
            ax5.axhline(y=max_entries, color='red', linestyle='--', alpha=0.5, label='Max')
            ax5.set_xlabel("Flash Page")
            ax5.set_ylabel("Entries")
            ax5.set_title("Entries Per Page")
            ax5.legend(fontsize=8)
        else:
            # Estimate from entry count
            est_pages = min(8, max(1, len(self.entries) // 85 + 1))
            ax5.bar(range(56, 56 + est_pages),
                    [min(85, len(self.entries) - i*85) for i in range(est_pages)],
                    color='#4CAF50', edgecolor='#333')
            ax5.set_xlabel("Flash Page")
            ax5.set_ylabel("Entries")
            ax5.set_title("Estimated Page Usage")

        # 6. Statistics box
        ax6 = fig.add_subplot(gs[2, 2])
        ax6.axis('off')
        stats_text = (
            f"Data Logger Stats\n"
            f"{'─'*25}\n"
            f"Total entries : {len(self.entries)}\n"
            f"Flash usage   : {usage}%\n"
            f"Page erases   : {self.page_erases}\n"
            f"Wrapped       : {'Yes' if self.wrapped else 'No'}\n"
            f"Anomalies     : {len(self.anomalies)}\n"
        )
        if temps:
            stats_text += (
                f"Temp range    : {min(temps):.1f}-{max(temps):.1f}°C\n"
                f"Humid range   : {min(humids):.1f}-{max(humids):.1f}%\n"
            )
        if t_sec and len(t_sec) > 1:
            stats_text += f"Duration      : {t_sec[-1]:.1f} sec\n"

        ax6.text(0.1, 0.95, stats_text, transform=ax6.transAxes, fontsize=9,
                 verticalalignment='top', fontfamily='monospace',
                 bbox=dict(boxstyle='round', facecolor='#F5F5F5', alpha=0.8))

        plt.savefig("data_logger_debug.png", dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to data_logger_debug.png")
        plt.show()


def generate_sample_data():
    """Generate sample data for testing."""
    import math
    import random

    lines = [
        "[LOG] Total entries    : 128 (flash: 128, RAM buffer: 0)",
        "[LOG] Flash usage      : 1536 / 8192 bytes (18%)",
        "[LOG] Page erases      : 0",
        "[LOG] Wrapped          : NO",
    ]

    # Page info
    lines.append("  56 │ 0x0800E000 │  85/85  │ FULL")
    lines.append("  57 │ 0x0800E400 │  43/85  │ WRITING")
    lines.append("  58 │ 0x0800E800 │   0/85  │ EMPTY")
    lines.append("  59 │ 0x0800EC00 │   0/85  │ EMPTY")
    lines.append("  60 │ 0x0800F000 │   0/85  │ EMPTY")
    lines.append("  61 │ 0x0800F400 │   0/85  │ EMPTY")
    lines.append("  62 │ 0x0800F800 │   0/85  │ EMPTY")
    lines.append("  63 │ 0x0800FC00 │   0/85  │ EMPTY")

    # CSV data
    lines.append("[CSV_ALL_START]")
    lines.append("timestamp_ms,temperature_c,humidity_pct,adc_raw")
    adc = 0
    for i in range(128):
        ts = 1000 + i * 500
        temp = 25.0 + 5.0 * math.sin(i * 0.05)
        humid = 65.0 + random.uniform(-3, 3)
        adc = (adc + 64) % 4096
        lines.append(f"{ts},{temp:.2f},{humid:.2f},{adc}")
    lines.append("[CSV_ALL_END]")

    return "\n".join(lines)


if __name__ == "__main__":
    debugger = DataLoggerDebugger()

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

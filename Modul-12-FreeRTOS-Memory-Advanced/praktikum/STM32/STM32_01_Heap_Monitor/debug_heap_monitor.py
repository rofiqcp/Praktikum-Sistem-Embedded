#!/usr/bin/env python3
"""
debug_heap_monitor.py
=====================
Parses serial output from STM32_01_Heap_Monitor.
Plots heap usage over time, logs to CSV, and provides real-time display.

Usage:
    python debug_heap_monitor.py                     # Default COM port auto-detect
    python debug_heap_monitor.py --port /dev/ttyUSB0
    python debug_heap_monitor.py --port COM3 --baud 115200
    python debug_heap_monitor.py --csv heap_log.csv  # Custom CSV filename
"""

import argparse
import csv
import os
import re
import sys
import time
from datetime import datetime
from collections import deque

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    HAS_MATPLOTLIB = True
except ImportError:
    print("WARNING: matplotlib not installed. Plotting disabled.")
    print("         Run: pip install matplotlib")
    HAS_MATPLOTLIB = False


# ── Regex patterns for parsing serial output ───────────────────────────────
RE_HEAP = re.compile(
    r'\[HEAP\]\s+Cycle=(\d+)\s*\|\s*Free=(\d+)\s*\|\s*Used=(\d+)\s*\|'
    r'\s*MinEver=(\d+)\s*\|\s*Usage=(\d+)%'
)
RE_HEAP_DETAIL = re.compile(
    r'\[HEAP\]\s+ActiveSlots=(\d+)\s*\|\s*ActiveBytes=(\d+)\s*\|'
    r'\s*TotalAllocs=(\d+)\s*\|\s*TotalFrees=(\d+)'
)
RE_HEAP_WATERMARK = re.compile(
    r'\[HEAP\]\s+HeapMon stack watermark:\s*(\d+)\s*words'
)
RE_ALLOC = re.compile(
    r'\[ALLOC\]\s+Slot\s+(\d+):\s*(\d+)\s*bytes\s*@\s*(0x[0-9A-Fa-f]+)\s*\|\s*Free=(\d+)'
)
RE_FREE = re.compile(
    r'\[FREE\]\s+Slot\s+(\d+):\s*(\d+)\s*bytes\s*@\s*(0x[0-9A-Fa-f]+)\s*\|\s*Free=(\d+)'
)


class HeapMonitor:
    """Collects and visualises FreeRTOS heap telemetry."""

    def __init__(self, csv_path="heap_monitor_log.csv", max_points=500):
        self.csv_path = csv_path
        self.max_points = max_points

        # Time-series buffers
        self.timestamps = deque(maxlen=max_points)
        self.free_heap = deque(maxlen=max_points)
        self.used_heap = deque(maxlen=max_points)
        self.min_ever = deque(maxlen=max_points)
        self.usage_pct = deque(maxlen=max_points)
        self.active_slots = deque(maxlen=max_points)
        self.active_bytes = deque(maxlen=max_points)

        # Alloc/free event log
        self.alloc_events = []
        self.free_events = []

        self.start_time = time.time()
        self._init_csv()

    def _init_csv(self):
        """Create CSV with header."""
        with open(self.csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'timestamp', 'elapsed_s', 'cycle', 'free_heap', 'used_heap',
                'min_ever', 'usage_pct', 'active_slots', 'active_bytes',
                'total_allocs', 'total_frees', 'event_type', 'event_detail'
            ])
        print(f"[CSV] Logging to {self.csv_path}")

    def _append_csv(self, row):
        with open(self.csv_path, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(row)

    def process_line(self, line):
        """Parse a single serial line and update buffers."""
        elapsed = time.time() - self.start_time
        now_str = datetime.now().strftime('%H:%M:%S.%f')[:-3]

        # ── HEAP status line ──
        m = RE_HEAP.search(line)
        if m:
            cycle, free, used, minev, pct = (int(x) for x in m.groups())
            self.timestamps.append(elapsed)
            self.free_heap.append(free)
            self.used_heap.append(used)
            self.min_ever.append(minev)
            self.usage_pct.append(pct)
            self._append_csv([
                now_str, f"{elapsed:.2f}", cycle, free, used,
                minev, pct, '', '', '', '', 'HEAP', ''
            ])
            return

        # ── HEAP detail line ──
        m = RE_HEAP_DETAIL.search(line)
        if m:
            slots, abytes, talloc, tfree = (int(x) for x in m.groups())
            self.active_slots.append(slots)
            self.active_bytes.append(abytes)
            self._append_csv([
                now_str, f"{elapsed:.2f}", '', '', '',
                '', '', slots, abytes, talloc, tfree, 'DETAIL', ''
            ])
            return

        # ── ALLOC event ──
        m = RE_ALLOC.search(line)
        if m:
            slot, size, addr, free = m.group(1), m.group(2), m.group(3), m.group(4)
            self.alloc_events.append({
                'time': elapsed, 'slot': int(slot),
                'size': int(size), 'addr': addr, 'free': int(free)
            })
            self._append_csv([
                now_str, f"{elapsed:.2f}", '', '', '',
                '', '', '', '', '', '', 'ALLOC',
                f"slot={slot} size={size} addr={addr} free={free}"
            ])
            return

        # ── FREE event ──
        m = RE_FREE.search(line)
        if m:
            slot, size, addr, free = m.group(1), m.group(2), m.group(3), m.group(4)
            self.free_events.append({
                'time': elapsed, 'slot': int(slot),
                'size': int(size), 'addr': addr, 'free': int(free)
            })
            self._append_csv([
                now_str, f"{elapsed:.2f}", '', '', '',
                '', '', '', '', '', '', 'FREE',
                f"slot={slot} size={size} addr={addr} free={free}"
            ])
            return

    def plot_summary(self):
        """Generate a static summary plot from collected data."""
        if not HAS_MATPLOTLIB or len(self.timestamps) < 2:
            print("[PLOT] Not enough data or matplotlib unavailable.")
            return

        fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
        fig.suptitle('FreeRTOS Heap Monitor – STM32_01', fontsize=14)

        t = list(self.timestamps)

        # Plot 1: Heap free / used / min-ever
        ax = axes[0]
        ax.plot(t, list(self.free_heap), 'g-', label='Free')
        ax.plot(t, list(self.used_heap), 'r-', label='Used')
        ax.plot(t, list(self.min_ever), 'b--', label='Min-ever free')
        ax.set_ylabel('Bytes')
        ax.set_title('Heap Usage Over Time')
        ax.legend(loc='upper right')
        ax.grid(True, alpha=0.3)

        # Plot 2: Usage percentage
        ax = axes[1]
        ax.plot(t, list(self.usage_pct), 'm-')
        ax.set_ylabel('Usage %')
        ax.set_title('Heap Usage Percentage')
        ax.set_ylim(0, 100)
        ax.grid(True, alpha=0.3)

        # Plot 3: Alloc / free events
        ax = axes[2]
        if self.alloc_events:
            at = [e['time'] for e in self.alloc_events]
            asizes = [e['size'] for e in self.alloc_events]
            ax.bar(at, asizes, width=0.3, color='red', alpha=0.7, label='Alloc')
        if self.free_events:
            ft = [e['time'] for e in self.free_events]
            fsizes = [-e['size'] for e in self.free_events]
            ax.bar(ft, fsizes, width=0.3, color='green', alpha=0.7, label='Free')
        ax.set_ylabel('Bytes')
        ax.set_xlabel('Time (s)')
        ax.set_title('Allocation / Free Events')
        ax.legend(loc='upper right')
        ax.grid(True, alpha=0.3)

        plt.tight_layout()
        png_path = self.csv_path.replace('.csv', '.png')
        plt.savefig(png_path, dpi=150)
        print(f"[PLOT] Saved to {png_path}")
        plt.show()


def auto_detect_port():
    """Try to find a USB-serial port."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or '').lower()
        if any(k in desc for k in ['usb', 'uart', 'serial', 'ch340', 'cp210', 'ftdi']):
            return p.device
    if ports:
        return ports[0].device
    return None


def main():
    parser = argparse.ArgumentParser(description='FreeRTOS Heap Monitor Debug Tool')
    parser.add_argument('--port', type=str, default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--csv', type=str, default='heap_monitor_log.csv', help='CSV log file')
    parser.add_argument('--duration', type=int, default=0, help='Run duration in seconds (0=forever)')
    parser.add_argument('--plot', action='store_true', help='Show plot after capture')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    if port is None:
        print("ERROR: No serial port found. Use --port to specify.")
        sys.exit(1)

    monitor = HeapMonitor(csv_path=args.csv)

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
                print(f"\n[INFO] Duration {args.duration}s reached. Stopping.")
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
        print(f"[INFO] Serial closed. {len(monitor.timestamps)} heap samples collected.")
        print(f"[INFO] {len(monitor.alloc_events)} alloc events, "
              f"{len(monitor.free_events)} free events.")

    if args.plot:
        monitor.plot_summary()


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
==========================================================================
FILE        : debug_multi_slave.py
PROJECT     : ESP32_07_SPI_Multi_Slave
MODUL       : 07 - SPI & Storage
DESCRIPTION : Parse serial output from multi-slave SPI demo, track
              per-slave communication statistics and timing analysis.

USAGE:
    python3 debug_multi_slave.py [PORT] [BAUD]
    python3 debug_multi_slave.py /dev/ttyUSB0 115200

SERIAL FORMATS EXPECTED:
    SLAVE:<id>,TX:<hex>,RX:<hex>,TIME:<us>
    B2B:S1=<us>,S2=<us>,TOTAL=<us>,OVERHEAD=<us>
==========================================================================
"""

import sys
import re
import time
import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from datetime import datetime

# ======================== Configuration ========================
DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200
MAX_DATA_POINTS = 200
UPDATE_INTERVAL = 200  # ms

# Regex patterns
SLAVE_PATTERN = re.compile(r'SLAVE:(\d+),TX:([0-9A-Fa-f]+),RX:([0-9A-Fa-f]+),TIME:(\d+)')
B2B_PATTERN = re.compile(r'B2B:S1=(\d+),S2=(\d+),TOTAL=(\d+),OVERHEAD=(\d+)')


class SlaveStats:
    """Per-slave communication statistics."""

    def __init__(self, slave_id, name):
        self.slave_id = slave_id
        self.name = name
        self.tx_count = 0
        self.rx_count = 0
        self.error_count = 0
        self.timings = deque(maxlen=MAX_DATA_POINTS)
        self.timestamps = deque(maxlen=MAX_DATA_POINTS)
        self.tx_data_log = []
        self.rx_data_log = []

    def add_transfer(self, tx_hex, rx_hex, time_us, timestamp):
        self.tx_count += 1
        self.rx_count += 1
        self.timings.append(time_us)
        self.timestamps.append(timestamp)
        self.tx_data_log.append(tx_hex)
        self.rx_data_log.append(rx_hex)

    @property
    def avg_time(self):
        return sum(self.timings) / len(self.timings) if self.timings else 0

    @property
    def min_time(self):
        return min(self.timings) if self.timings else 0

    @property
    def max_time(self):
        return max(self.timings) if self.timings else 0


class MultiSlaveMonitor:
    """Real-time monitoring for multi-slave SPI communication."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.serial_conn = None
        self.start_time = time.time()

        # Per-slave statistics
        self.slaves = {
            1: SlaveStats(1, "ADC-Sensor"),
            2: SlaveStats(2, "Flash-Memory"),
        }

        # Back-to-back data
        self.b2b_overhead = deque(maxlen=MAX_DATA_POINTS)
        self.b2b_timestamps = deque(maxlen=MAX_DATA_POINTS)

    def connect(self):
        """Connect to serial port."""
        try:
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=1)
            print(f"[OK] Connected to {self.port} at {self.baud} baud")
            time.sleep(2)
            self.serial_conn.reset_input_buffer()
            return True
        except serial.SerialException as e:
            print(f"[ERROR] Cannot connect to {self.port}: {e}")
            ports = serial.tools.list_ports.comports()
            if ports:
                print("Available ports:")
                for p in ports:
                    print(f"  {p.device} - {p.description}")
            return False

    def parse_line(self, line):
        """Parse a serial line for slave or B2B data."""
        line = line.strip()
        elapsed = time.time() - self.start_time

        # Check for slave transfer data
        match = SLAVE_PATTERN.search(line)
        if match:
            slave_id = int(match.group(1))
            tx_hex = match.group(2)
            rx_hex = match.group(3)
            time_us = int(match.group(4))
            if slave_id in self.slaves:
                self.slaves[slave_id].add_transfer(tx_hex, rx_hex, time_us, elapsed)
                return ('slave', slave_id)

        # Check for back-to-back data
        match = B2B_PATTERN.search(line)
        if match:
            overhead = int(match.group(4))
            self.b2b_overhead.append(overhead)
            self.b2b_timestamps.append(elapsed)
            return ('b2b', overhead)

        return None

    def read_serial(self):
        """Read and parse serial data."""
        if self.serial_conn and self.serial_conn.in_waiting:
            try:
                line = self.serial_conn.readline().decode('utf-8', errors='ignore')
                return self.parse_line(line)
            except (serial.SerialException, UnicodeDecodeError):
                pass
        return None

    def setup_plot(self):
        """Setup matplotlib figure with subplots."""
        self.fig, axes = plt.subplots(2, 2, figsize=(14, 9))
        self.fig.suptitle("Multi-Slave SPI Communication Monitor",
                          fontsize=14, fontweight='bold')

        # Top-left: Slave 1 timing
        self.ax_s1 = axes[0][0]
        self.line_s1, = self.ax_s1.plot([], [], 'b-o', markersize=3, label="Slave 1")
        self.ax_s1.set_title(f"Slave 1 ({self.slaves[1].name}) Timing")
        self.ax_s1.set_ylabel("Time (µs)")
        self.ax_s1.grid(True, alpha=0.3)
        self.ax_s1.legend()

        # Top-right: Slave 2 timing
        self.ax_s2 = axes[0][1]
        self.line_s2, = self.ax_s2.plot([], [], 'r-o', markersize=3, label="Slave 2")
        self.ax_s2.set_title(f"Slave 2 ({self.slaves[2].name}) Timing")
        self.ax_s2.set_ylabel("Time (µs)")
        self.ax_s2.grid(True, alpha=0.3)
        self.ax_s2.legend()

        # Bottom-left: Comparison overlay
        self.ax_cmp = axes[1][0]
        self.line_cmp1, = self.ax_cmp.plot([], [], 'b-', linewidth=1.5, label="Slave 1")
        self.line_cmp2, = self.ax_cmp.plot([], [], 'r-', linewidth=1.5, label="Slave 2")
        self.line_b2b, = self.ax_cmp.plot([], [], 'g--', linewidth=1.0, label="B2B Overhead")
        self.ax_cmp.set_title("Timing Comparison")
        self.ax_cmp.set_xlabel("Time (s)")
        self.ax_cmp.set_ylabel("Time (µs)")
        self.ax_cmp.grid(True, alpha=0.3)
        self.ax_cmp.legend()

        # Bottom-right: Statistics text
        self.ax_stats = axes[1][1]
        self.ax_stats.axis('off')
        self.stats_text = self.ax_stats.text(0.05, 0.95, "", transform=self.ax_stats.transAxes,
                                              fontsize=9, verticalalignment='top',
                                              fontfamily='monospace')

        plt.tight_layout()

    def animate(self, frame):
        """Animation callback."""
        for _ in range(10):
            self.read_serial()

        # Update Slave 1 plot
        s1 = self.slaves[1]
        if s1.timestamps:
            self.line_s1.set_data(list(s1.timestamps), list(s1.timings))
            self.ax_s1.relim()
            self.ax_s1.autoscale_view()

        # Update Slave 2 plot
        s2 = self.slaves[2]
        if s2.timestamps:
            self.line_s2.set_data(list(s2.timestamps), list(s2.timings))
            self.ax_s2.relim()
            self.ax_s2.autoscale_view()

        # Update comparison plot
        if s1.timestamps:
            self.line_cmp1.set_data(list(s1.timestamps), list(s1.timings))
        if s2.timestamps:
            self.line_cmp2.set_data(list(s2.timestamps), list(s2.timings))
        if self.b2b_timestamps:
            self.line_b2b.set_data(list(self.b2b_timestamps), list(self.b2b_overhead))
        self.ax_cmp.relim()
        self.ax_cmp.autoscale_view()

        # Update statistics
        lines = ["═══ Communication Statistics ═══\n"]
        for sid, s in self.slaves.items():
            lines.append(f"Slave {sid} ({s.name}):")
            lines.append(f"  Transfers : {s.tx_count}")
            lines.append(f"  Avg Time  : {s.avg_time:.1f} µs")
            lines.append(f"  Min Time  : {s.min_time} µs")
            lines.append(f"  Max Time  : {s.max_time} µs")
            lines.append("")

        if self.b2b_overhead:
            avg_oh = sum(self.b2b_overhead) / len(self.b2b_overhead)
            lines.append(f"Back-to-Back Overhead:")
            lines.append(f"  Avg: {avg_oh:.1f} µs")
            lines.append(f"  Min: {min(self.b2b_overhead)} µs")
            lines.append(f"  Max: {max(self.b2b_overhead)} µs")

        elapsed = time.time() - self.start_time
        lines.append(f"\nElapsed: {elapsed:.1f}s")
        self.stats_text.set_text("\n".join(lines))

        return [self.line_s1, self.line_s2, self.line_cmp1,
                self.line_cmp2, self.line_b2b, self.stats_text]

    def run(self):
        """Main entry point."""
        if not self.connect():
            return

        self.setup_plot()

        ani = animation.FuncAnimation(
            self.fig, self.animate, interval=UPDATE_INTERVAL,
            blit=False, cache_frame_data=False
        )

        print("[INFO] Plotting started. Close the window to exit.")
        try:
            plt.show()
        except KeyboardInterrupt:
            print("\n[INFO] Stopped by user.")
        finally:
            if self.serial_conn:
                self.serial_conn.close()
                print("[OK] Serial connection closed.")


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

    print("=" * 50)
    print(" Multi-Slave SPI Communication Monitor")
    print(f" Port: {port}  Baud: {baud}")
    print("=" * 50)

    monitor = MultiSlaveMonitor(port, baud)
    monitor.run()


if __name__ == "__main__":
    main()

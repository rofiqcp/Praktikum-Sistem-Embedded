#!/usr/bin/env python3
"""
==========================================================================
FILE        : debug_mcp3208.py
PROJECT     : ESP32_05_SPI_MCP3208_ADC
MODUL       : 07 - SPI & Storage
DESCRIPTION : Parse serial ADC data from MCP3208 and plot real-time
              voltage graph per channel using matplotlib with live update.

USAGE:
    python3 debug_mcp3208.py [PORT] [BAUD]
    python3 debug_mcp3208.py /dev/ttyUSB0 115200

SERIAL FORMAT EXPECTED:
    DATA:raw0,raw1,raw2,raw3,raw4,raw5,raw6,raw7
    Example: DATA:2048,1024,512,4095,0,3000,1500,2000
==========================================================================
"""

import sys
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
VREF = 3.3
ADC_MAX = 4095
NUM_CHANNELS = 8
MAX_DATA_POINTS = 200   # Number of points to display on graph
UPDATE_INTERVAL = 100   # milliseconds between plot updates

# Channel colors
CHANNEL_COLORS = [
    '#FF0000', '#00AA00', '#0000FF', '#FF8800',
    '#AA00AA', '#00AAAA', '#888800', '#FF00FF'
]

CHANNEL_LABELS = [f"CH{i}" for i in range(NUM_CHANNELS)]


class MCP3208Monitor:
    """Real-time monitoring and plotting for MCP3208 ADC data."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.serial_conn = None

        # Data storage: deque per channel for rolling window
        self.voltage_data = [deque(maxlen=MAX_DATA_POINTS) for _ in range(NUM_CHANNELS)]
        self.time_data = deque(maxlen=MAX_DATA_POINTS)
        self.start_time = time.time()
        self.sample_count = 0

        # Statistics
        self.ch_min = [VREF] * NUM_CHANNELS
        self.ch_max = [0.0] * NUM_CHANNELS
        self.ch_sum = [0.0] * NUM_CHANNELS

    def connect(self):
        """Connect to serial port."""
        try:
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=1)
            print(f"[OK] Connected to {self.port} at {self.baud} baud")
            time.sleep(2)  # Wait for ESP32 reset
            self.serial_conn.reset_input_buffer()
            return True
        except serial.SerialException as e:
            print(f"[ERROR] Cannot connect to {self.port}: {e}")
            self.list_ports()
            return False

    @staticmethod
    def list_ports():
        """List available serial ports."""
        ports = serial.tools.list_ports.comports()
        if ports:
            print("\nAvailable ports:")
            for p in ports:
                print(f"  {p.device} - {p.description}")
        else:
            print("\nNo serial ports found!")

    def raw_to_voltage(self, raw):
        """Convert raw ADC value to voltage."""
        return (raw / ADC_MAX) * VREF

    def parse_line(self, line):
        """Parse a serial line for ADC data.
        Expected format: DATA:raw0,raw1,...,raw7
        """
        line = line.strip()
        if not line.startswith("DATA:"):
            return None

        try:
            data_str = line[5:]  # Remove "DATA:" prefix
            raw_values = [int(x) for x in data_str.split(",")]
            if len(raw_values) == NUM_CHANNELS:
                return raw_values
        except (ValueError, IndexError):
            pass
        return None

    def update_data(self, raw_values):
        """Update data storage and statistics with new readings."""
        elapsed = time.time() - self.start_time
        self.time_data.append(elapsed)
        self.sample_count += 1

        for ch in range(NUM_CHANNELS):
            voltage = self.raw_to_voltage(raw_values[ch])
            self.voltage_data[ch].append(voltage)

            # Update statistics
            if voltage < self.ch_min[ch]:
                self.ch_min[ch] = voltage
            if voltage > self.ch_max[ch]:
                self.ch_max[ch] = voltage
            self.ch_sum[ch] += voltage

    def read_serial(self):
        """Read and parse one line from serial."""
        if self.serial_conn and self.serial_conn.in_waiting:
            try:
                line = self.serial_conn.readline().decode('utf-8', errors='ignore')
                return self.parse_line(line)
            except (serial.SerialException, UnicodeDecodeError):
                pass
        return None

    def setup_plot(self):
        """Setup matplotlib figure with subplots."""
        self.fig, (self.ax_main, self.ax_stats) = plt.subplots(
            2, 1, figsize=(14, 9),
            gridspec_kw={'height_ratios': [3, 1]}
        )
        self.fig.suptitle("MCP3208 8-Channel ADC Monitor", fontsize=14, fontweight='bold')

        # Main voltage plot
        self.lines = []
        for ch in range(NUM_CHANNELS):
            line, = self.ax_main.plot([], [], color=CHANNEL_COLORS[ch],
                                      label=CHANNEL_LABELS[ch], linewidth=1.2)
            self.lines.append(line)

        self.ax_main.set_xlabel("Time (s)")
        self.ax_main.set_ylabel("Voltage (V)")
        self.ax_main.set_ylim(-0.1, VREF + 0.2)
        self.ax_main.legend(loc='upper right', ncol=4, fontsize=8)
        self.ax_main.grid(True, alpha=0.3)
        self.ax_main.set_title("Real-Time Voltage Readings")

        # Statistics text area
        self.ax_stats.axis('off')
        self.stats_text = self.ax_stats.text(0.02, 0.95, "", transform=self.ax_stats.transAxes,
                                              fontsize=8, verticalalignment='top',
                                              fontfamily='monospace')

        plt.tight_layout()

    def animate(self, frame):
        """Animation callback for live plot update."""
        # Read multiple lines per frame for responsiveness
        for _ in range(5):
            raw_values = self.read_serial()
            if raw_values:
                self.update_data(raw_values)

        # Update plot lines
        if len(self.time_data) > 0:
            time_list = list(self.time_data)
            for ch in range(NUM_CHANNELS):
                data_list = list(self.voltage_data[ch])
                self.lines[ch].set_data(time_list[:len(data_list)], data_list)

            self.ax_main.set_xlim(max(0, time_list[-1] - 30), time_list[-1] + 1)

        # Update statistics text
        if self.sample_count > 0:
            stats_lines = [f"{'Channel':<8} {'Min (V)':<10} {'Max (V)':<10} {'Avg (V)':<10} {'Latest (V)':<10}"]
            stats_lines.append("-" * 50)
            for ch in range(NUM_CHANNELS):
                avg = self.ch_sum[ch] / self.sample_count if self.sample_count > 0 else 0
                latest = self.voltage_data[ch][-1] if self.voltage_data[ch] else 0
                stats_lines.append(
                    f"CH{ch:<6} {self.ch_min[ch]:<10.4f} {self.ch_max[ch]:<10.4f} {avg:<10.4f} {latest:<10.4f}"
                )
            stats_lines.append(f"\nSamples: {self.sample_count}  |  "
                               f"Time: {time.time() - self.start_time:.1f}s")
            self.stats_text.set_text("\n".join(stats_lines))

        return self.lines + [self.stats_text]

    def run(self):
        """Main entry point: connect, setup plot, start animation."""
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
    print(" MCP3208 ADC Real-Time Monitor")
    print(f" Port: {port}  Baud: {baud}")
    print("=" * 50)

    monitor = MCP3208Monitor(port, baud)
    monitor.run()


if __name__ == "__main__":
    main()

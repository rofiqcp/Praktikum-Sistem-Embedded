#!/usr/bin/env python3
"""
==========================================================================
FILE        : debug_dac_mcp4921.py
PROJECT     : ESP32_06_SPI_DAC_MCP4921
MODUL       : 07 - SPI & Storage
DESCRIPTION : Parse serial DAC values from MCP4921 waveform generator,
              plot waveform output, calculate frequency and amplitude.

USAGE:
    python3 debug_dac_mcp4921.py [PORT] [BAUD]
    python3 debug_dac_mcp4921.py /dev/ttyUSB0 115200

SERIAL FORMAT EXPECTED:
    WAVE:<type>,DAC:<value>,VOLT:<voltage>
    Example: WAVE:SINE,DAC:2048,VOLT:1.6500
==========================================================================
"""

import sys
import time
import re
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
DAC_MAX = 4095
MAX_DATA_POINTS = 500
UPDATE_INTERVAL = 100  # ms

WAVEFORM_COLORS = {
    'SINE': '#0066FF',
    'SAWTOOTH': '#FF6600',
    'TRIANGLE': '#00AA00',
    'SQUARE': '#FF0000',
}

# Regex for parsing serial data
DATA_PATTERN = re.compile(r'WAVE:(\w+),DAC:(\d+),VOLT:([\d.]+)')


class DACMonitor:
    """Real-time monitoring and analysis for MCP4921 DAC output."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.serial_conn = None

        # Data storage
        self.dac_values = deque(maxlen=MAX_DATA_POINTS)
        self.voltages = deque(maxlen=MAX_DATA_POINTS)
        self.timestamps = deque(maxlen=MAX_DATA_POINTS)
        self.waveform_types = deque(maxlen=MAX_DATA_POINTS)
        self.start_time = time.time()

        # Analysis
        self.current_waveform = "UNKNOWN"
        self.sample_count = 0
        self.waveform_change_times = []

        # Frequency analysis
        self.last_zero_cross = None
        self.zero_cross_count = 0
        self.estimated_freq = 0.0

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
        """Parse serial line for DAC data."""
        line = line.strip()
        match = DATA_PATTERN.search(line)
        if match:
            return {
                'waveform': match.group(1),
                'dac': int(match.group(2)),
                'voltage': float(match.group(3))
            }
        return None

    def update_data(self, data):
        """Update data storage with new readings."""
        now = time.time() - self.start_time
        self.timestamps.append(now)
        self.dac_values.append(data['dac'])
        self.voltages.append(data['voltage'])
        self.waveform_types.append(data['waveform'])
        self.sample_count += 1

        # Detect waveform change
        if data['waveform'] != self.current_waveform:
            self.current_waveform = data['waveform']
            self.waveform_change_times.append(now)
            print(f"[WAVE] Switched to: {self.current_waveform} at t={now:.2f}s")

        # Simple zero-crossing frequency estimation
        if len(self.dac_values) >= 2:
            mid = DAC_MAX / 2
            prev = self.dac_values[-2]
            curr = self.dac_values[-1]
            if (prev < mid <= curr) or (prev >= mid > curr):
                self.zero_cross_count += 1
                if self.last_zero_cross is not None:
                    period = (now - self.last_zero_cross) * 2  # Half-period to full
                    if period > 0:
                        self.estimated_freq = 1.0 / period
                self.last_zero_cross = now

    def calculate_amplitude(self):
        """Calculate peak-to-peak amplitude from recent data."""
        if len(self.voltages) < 10:
            return 0.0, 0.0, 0.0
        recent = list(self.voltages)[-100:]
        v_min = min(recent)
        v_max = max(recent)
        return v_min, v_max, v_max - v_min

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
        """Setup matplotlib figure."""
        self.fig, (self.ax_wave, self.ax_dac, self.ax_info) = plt.subplots(
            3, 1, figsize=(14, 10),
            gridspec_kw={'height_ratios': [2, 2, 1]}
        )
        self.fig.suptitle("MCP4921 DAC Waveform Monitor", fontsize=14, fontweight='bold')

        # Voltage plot
        self.line_volt, = self.ax_wave.plot([], [], color='#0066FF', linewidth=1.5)
        self.ax_wave.set_ylabel("Voltage (V)")
        self.ax_wave.set_ylim(-0.1, VREF + 0.2)
        self.ax_wave.grid(True, alpha=0.3)
        self.ax_wave.set_title("DAC Output Voltage")

        # DAC raw plot
        self.line_dac, = self.ax_dac.plot([], [], color='#FF6600', linewidth=1.0)
        self.ax_dac.set_xlabel("Time (s)")
        self.ax_dac.set_ylabel("DAC Value")
        self.ax_dac.set_ylim(-100, DAC_MAX + 200)
        self.ax_dac.grid(True, alpha=0.3)
        self.ax_dac.set_title("Raw DAC Values")

        # Info text
        self.ax_info.axis('off')
        self.info_text = self.ax_info.text(0.02, 0.95, "", transform=self.ax_info.transAxes,
                                            fontsize=10, verticalalignment='top',
                                            fontfamily='monospace')

        plt.tight_layout()

    def animate(self, frame):
        """Animation callback."""
        for _ in range(10):
            data = self.read_serial()
            if data:
                self.update_data(data)

        if len(self.timestamps) > 1:
            ts = list(self.timestamps)
            vs = list(self.voltages)
            ds = list(self.dac_values)

            # Update voltage plot
            color = WAVEFORM_COLORS.get(self.current_waveform, '#0066FF')
            self.line_volt.set_data(ts, vs)
            self.line_volt.set_color(color)

            # Update DAC plot
            self.line_dac.set_data(ts, ds)
            self.line_dac.set_color(color)

            # Adjust x-axis
            window = 5.0  # Show last 5 seconds
            x_max = ts[-1] + 0.5
            x_min = max(0, x_max - window)
            self.ax_wave.set_xlim(x_min, x_max)
            self.ax_dac.set_xlim(x_min, x_max)

        # Update info
        v_min, v_max, vpp = self.calculate_amplitude()
        info_lines = [
            f"Waveform : {self.current_waveform}",
            f"Samples  : {self.sample_count}",
            f"Frequency: {self.estimated_freq:.2f} Hz (estimated)",
            f"Amplitude: Vmin={v_min:.3f}V  Vmax={v_max:.3f}V  Vpp={vpp:.3f}V",
            f"Elapsed  : {time.time() - self.start_time:.1f}s",
        ]
        self.info_text.set_text("\n".join(info_lines))

        return [self.line_volt, self.line_dac, self.info_text]

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
    print(" MCP4921 DAC Waveform Monitor")
    print(f" Port: {port}  Baud: {baud}")
    print("=" * 50)

    monitor = DACMonitor(port, baud)
    monitor.run()


if __name__ == "__main__":
    main()

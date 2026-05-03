#!/usr/bin/env python3
"""
Debug Script: Storage Data Logger Monitor
Modul 07 - SPI & Storage | Program 12

Fitur:
- Parse data CSV dari output serial (timestamp, temperature, humidity, ADC)
- Real-time plot: temperature, humidity, ADC value
- Storage usage gauge
- Data rate calculation dan statistik

Penggunaan:
    python debug_data_logger.py [PORT] [BAUDRATE]
    python debug_data_logger.py /dev/ttyUSB0 115200
"""

import sys
import re
import time
import math
from collections import deque

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.patches import Wedge, Circle
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class DataLoggerDebugger:
    """Parser dan visualizer untuk data logger."""

    def __init__(self, max_points=500):
        self.max_points = max_points
        self.timestamps = deque(maxlen=max_points)
        self.temperatures = deque(maxlen=max_points)
        self.humidities = deque(maxlen=max_points)
        self.adc_values = deque(maxlen=max_points)

        # Storage info
        self.storage_total = 0
        self.storage_used = 0
        self.storage_free = 0
        self.storage_history = []  # (time, used_pct)

        # Statistics
        self.entries_collected = 0
        self.entries_written = 0
        self.flushes = 0
        self.file_rotations = 0
        self.file_size = 0
        self.data_rate = 0.0

        # Timing
        self.start_time = time.time()
        self.sample_times = deque(maxlen=100)  # For rate calculation

    def parse_line(self, line):
        """Parse satu baris output serial."""
        now = time.time() - self.start_time

        # Parse sensor sample data
        sample_match = re.search(
            r'Sample\s*#(\d+):\s*T=([\d.-]+).*?H=([\d.-]+).*?ADC=(\d+)', line)
        if sample_match:
            idx = int(sample_match.group(1))
            temp = float(sample_match.group(2))
            hum = float(sample_match.group(3))
            adc = int(sample_match.group(4))

            self.timestamps.append(now)
            self.temperatures.append(temp)
            self.humidities.append(hum)
            self.adc_values.append(adc)
            self.sample_times.append(time.time())

            if idx % 10 == 0:
                print(f"  [DATA] #{idx}: T={temp:.1f}°C, H={hum:.1f}%, ADC={adc}")
            return

        # Parse storage info
        total_match = re.search(r'Total space\s*:\s*(\d+)\s*bytes', line)
        if total_match:
            self.storage_total = int(total_match.group(1))

        used_match = re.search(r'Used space\s*:\s*(\d+)\s*bytes', line)
        if used_match:
            self.storage_used = int(used_match.group(1))

        free_match = re.search(r'Free space\s*:\s*(\d+)\s*bytes', line)
        if free_match:
            self.storage_free = int(free_match.group(1))
            if self.storage_total > 0:
                pct = self.storage_used / self.storage_total * 100
                self.storage_history.append((now, pct))
                print(f"  [STORAGE] Used: {pct:.1f}% ({self.storage_used}B / {self.storage_total}B)")

        # Parse entries logged
        entries_match = re.search(r'Entries logged\s*:\s*(\d+)', line)
        if entries_match:
            self.entries_written = int(entries_match.group(1))

        # Parse flushes
        flush_match = re.search(r'Flushes\s*:\s*(\d+)', line)
        if flush_match:
            self.flushes = int(flush_match.group(1))

        # Parse file size from writer
        fsize_match = re.search(r'file:\s*(\d+)\s*bytes', line)
        if fsize_match:
            self.file_size = int(fsize_match.group(1))

        # Parse data rate
        rate_match = re.search(r'Data rate\s*:\s*([\d.]+)\s*entries/min', line)
        if rate_match:
            self.data_rate = float(rate_match.group(1))

        # Parse file rotations
        rotation_match = re.search(r'File rotations\s*:\s*(\d+)', line)
        if rotation_match:
            self.file_rotations = int(rotation_match.group(1))

        # Parse flushed entries
        flushed_match = re.search(r'Flushed\s*(\d+)\s*entries', line)
        if flushed_match:
            count = int(flushed_match.group(1))
            self.flushes += 1
            print(f"  [FLUSH] {count} entries written to file")

    def calculate_data_rate(self):
        """Calculate current data rate from sample times."""
        if len(self.sample_times) >= 2:
            dt = self.sample_times[-1] - self.sample_times[0]
            if dt > 0:
                return len(self.sample_times) / dt * 60.0  # entries per minute
        return 0.0

    def print_summary(self):
        """Print ringkasan data logger."""
        print("\n" + "=" * 60)
        print("        DATA LOGGER SUMMARY")
        print("=" * 60)

        elapsed = time.time() - self.start_time
        print(f"\n  Runtime: {elapsed:.1f}s")
        print(f"  Samples captured: {len(self.temperatures)}")
        print(f"  Entries written: {self.entries_written}")
        print(f"  Flushes: {self.flushes}")
        print(f"  File rotations: {self.file_rotations}")

        if self.temperatures:
            temps = list(self.temperatures)
            hums = list(self.humidities)
            adcs = list(self.adc_values)

            print(f"\n  Temperature: min={min(temps):.1f}, max={max(temps):.1f}, "
                  f"avg={sum(temps)/len(temps):.1f} °C")
            print(f"  Humidity:    min={min(hums):.1f}, max={max(hums):.1f}, "
                  f"avg={sum(hums)/len(hums):.1f} %")
            print(f"  ADC:         min={min(adcs)}, max={max(adcs)}, "
                  f"avg={sum(adcs)/len(adcs):.0f}")

        if self.storage_total > 0:
            print(f"\n  Storage: {self.storage_used}/{self.storage_total} bytes "
                  f"({self.storage_used/self.storage_total*100:.1f}%)")

        rate = self.calculate_data_rate()
        print(f"  Data rate: {rate:.1f} samples/min")

        print("=" * 60)

    def plot_results(self):
        """Generate visualisasi data logger."""
        if not HAS_MATPLOTLIB:
            print("[WARN] matplotlib not available, skipping plots")
            return

        if not self.timestamps:
            print("[WARN] No data to plot")
            return

        fig, axes = plt.subplots(2, 2, figsize=(16, 10))
        fig.suptitle('Storage Data Logger Analysis', fontsize=14, fontweight='bold')

        times = list(self.timestamps)

        # Plot 1: Temperature (sine wave expected)
        ax1 = axes[0][0]
        temps = list(self.temperatures)
        ax1.plot(times, temps, 'r-', linewidth=1.5, label='Temperature')
        ax1.fill_between(times, min(temps), temps, alpha=0.2, color='red')
        ax1.set_xlabel('Time (s)')
        ax1.set_ylabel('Temperature (°C)')
        ax1.set_title('Temperature (Sine Wave Simulation)')
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # Plot 2: Humidity (random expected)
        ax2 = axes[0][1]
        hums = list(self.humidities)
        ax2.plot(times, hums, 'b-', linewidth=1, alpha=0.7, label='Humidity')
        if len(hums) > 10:
            # Moving average
            window = min(10, len(hums))
            ma = np.convolve(hums, np.ones(window)/window, mode='valid')
            ma_times = times[window-1:]
            ax2.plot(ma_times, ma, 'b-', linewidth=2, label=f'MA({window})')
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('Humidity (%)')
        ax2.set_title('Humidity (Random Simulation)')
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        # Plot 3: ADC (sawtooth expected)
        ax3 = axes[1][0]
        adcs = list(self.adc_values)
        ax3.plot(times, adcs, 'g-', linewidth=1.5, label='ADC Value')
        ax3.set_xlabel('Time (s)')
        ax3.set_ylabel('ADC Value (0-4095)')
        ax3.set_title('ADC Value (Sawtooth Simulation)')
        ax3.set_ylim(-100, 4200)
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # Plot 4: Storage usage over time
        ax4 = axes[1][1]
        if self.storage_history:
            st_times = [s[0] for s in self.storage_history]
            st_pcts = [s[1] for s in self.storage_history]
            ax4.plot(st_times, st_pcts, 'mo-', linewidth=2, markersize=4)
            ax4.fill_between(st_times, 0, st_pcts, alpha=0.3, color='purple')
            ax4.set_ylim(0, 105)
            ax4.axhline(y=80, color='orange', linestyle='--', alpha=0.5, label='80% warning')
            ax4.axhline(y=95, color='red', linestyle='--', alpha=0.5, label='95% critical')
            ax4.legend()
        else:
            # Show a gauge-like visualization
            if self.storage_total > 0:
                pct = self.storage_used / self.storage_total * 100
                colors = ['#2ecc71' if pct < 60 else '#f39c12' if pct < 85 else '#e74c3c']
                ax4.barh(['Storage'], [pct], color=colors, edgecolor='black')
                ax4.barh(['Storage'], [100 - pct], left=[pct], color='lightgray', edgecolor='black')
                ax4.text(pct / 2, 0, f'{pct:.1f}%', ha='center', va='center',
                         fontweight='bold', fontsize=14)
                ax4.set_xlim(0, 100)
            else:
                ax4.text(0.5, 0.5, 'No storage data', ha='center', va='center',
                         transform=ax4.transAxes, fontsize=12)
        ax4.set_xlabel('Time (s)' if self.storage_history else 'Usage (%)')
        ax4.set_title('Storage Usage')
        ax4.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('data_logger_analysis.png', dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to data_logger_analysis.png")
        plt.show()


def generate_demo_data(debugger, duration=30, interval=1.0):
    """Generate demo data simulating what the ESP32 would produce."""
    print("[INFO] Generating demo data for visualization...")

    for i in range(int(duration / interval)):
        t_ms = int(i * interval * 1000)

        # Sine wave temperature
        temp = 25.0 + 10.0 * math.sin(2 * math.pi * t_ms / 60000.0)
        # Random humidity
        import random
        hum = 30.0 + random.random() * 50.0
        # Sawtooth ADC
        adc = int((t_ms % 30000) * 4095 / 30000)

        line = f"Sample #{i+1}: T={temp:.1f}°C, H={hum:.1f}%, ADC={adc} (buf: 5/64)"
        debugger.parse_line(line)

        # Periodic storage update
        if i % 10 == 0:
            used = 1024 + i * 50
            debugger.parse_line(f"Total space : 957314 bytes")
            debugger.parse_line(f"Used space  : {used} bytes")
            debugger.parse_line(f"Free space  : {957314 - used} bytes")
            debugger.parse_line(f"Entries logged  : {i}")
            debugger.parse_line(f"Data rate       : 60.0 entries/min")

        if i % 10 == 9:
            debugger.parse_line(f"Flushed 10 entries (file: {1024 + i * 50} bytes)")

        time.sleep(0.02)  # Speed up demo


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    debugger = DataLoggerDebugger()

    if not HAS_SERIAL:
        print("[INFO] Running in demo mode...")
        generate_demo_data(debugger, duration=60, interval=1.0)
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

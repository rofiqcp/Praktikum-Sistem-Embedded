#!/usr/bin/env python3
"""
============================================================================
Debug & Analysis Tool - ESP32_02_WiFi_Station
============================================================================
Modul 13 - Network & IoT | Praktikum Sistem Embedded

Tool ini memonitor output serial dari ESP32 WiFi Station,
melacak event koneksi, RSSI, dan status IP address.

PENGGUNAAN:
  python3 debug_analysis.py                    # Default /dev/ttyUSB0
  python3 debug_analysis.py --port COM3        # Windows
  python3 debug_analysis.py --file output.txt  # Parse dari file

DEPENDENSI:
  pip install pyserial matplotlib
============================================================================
"""

import argparse
import re
import sys
import time
from datetime import datetime
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARNING] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] matplotlib not installed. Install: pip install matplotlib")


class WiFiStationAnalyzer:
    """Analyzer untuk koneksi WiFi Station ESP32."""

    def __init__(self):
        self.events = []          # List of {timestamp, event_type, detail}
        self.rssi_history = []    # List of {timestamp, rssi}
        self.ip_address = None
        self.connected = False
        self.retry_count = 0
        self.connect_time = None
        self.disconnect_count = 0

        # Regex patterns
        self.ip_pattern = re.compile(r'IP Address\s*:\s*([\d.]+)')
        self.netmask_pattern = re.compile(r'Netmask\s*:\s*([\d.]+)')
        self.gateway_pattern = re.compile(r'Gateway\s*:\s*([\d.]+)')
        self.rssi_pattern = re.compile(r'RSSI:\s*(-?\d+)\s*dBm')
        self.retry_pattern = re.compile(r'Retry koneksi\s*\((\d+)/(\d+)\)')
        self.heap_pattern = re.compile(r'heap free:\s*(\d+)\s*bytes')
        self.disconnect_reason = re.compile(r'DISCONNECTED\s*\(reason:\s*(\d+)\)')

    def parse_line(self, line):
        """Parse satu baris output serial dan track events."""
        line = line.strip()
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

        # Track koneksi events
        if "STA_CONNECTED" in line and "DISCONNECTED" not in line:
            self.connected = True
            self.connect_time = timestamp
            self.events.append({
                'time': timestamp, 'type': 'CONNECTED',
                'detail': 'Terhubung ke AP'
            })
            return "CONNECTED"

        if "STA_DISCONNECTED" in line:
            self.connected = False
            self.disconnect_count += 1
            reason_match = self.disconnect_reason.search(line)
            reason = reason_match.group(1) if reason_match else "unknown"
            self.events.append({
                'time': timestamp, 'type': 'DISCONNECTED',
                'detail': f'Disconnect reason: {reason}'
            })
            return "DISCONNECTED"

        # Track retry
        retry_match = self.retry_pattern.search(line)
        if retry_match:
            self.retry_count = int(retry_match.group(1))
            self.events.append({
                'time': timestamp, 'type': 'RETRY',
                'detail': f'Retry {retry_match.group(1)}/{retry_match.group(2)}'
            })
            return "RETRY"

        # Track IP address
        ip_match = self.ip_pattern.search(line)
        if ip_match:
            self.ip_address = ip_match.group(1)
            self.events.append({
                'time': timestamp, 'type': 'GOT_IP',
                'detail': f'IP: {self.ip_address}'
            })
            return "GOT_IP"

        # Track RSSI dari monitoring
        rssi_match = self.rssi_pattern.search(line)
        if rssi_match:
            rssi = int(rssi_match.group(1))
            self.rssi_history.append({'time': timestamp, 'rssi': rssi})
            return "RSSI"

        # Track heap
        heap_match = self.heap_pattern.search(line)
        if heap_match:
            self.events.append({
                'time': timestamp, 'type': 'HEAP',
                'detail': f'{int(heap_match.group(1)):,} bytes free'
            })

        return None

    def print_summary(self):
        """Tampilkan ringkasan status koneksi."""
        print("\n" + "=" * 60)
        print("  ANALISIS KONEKSI WiFi STATION")
        print("=" * 60)
        print(f"  Status       : {'CONNECTED' if self.connected else 'DISCONNECTED'}")
        print(f"  IP Address   : {self.ip_address or 'N/A'}")
        print(f"  Total Retry  : {self.retry_count}")
        print(f"  Disconnects  : {self.disconnect_count}")

        if self.rssi_history:
            rssis = [r['rssi'] for r in self.rssi_history]
            avg_rssi = sum(rssis) / len(rssis)
            print(f"  RSSI (avg)   : {avg_rssi:.1f} dBm")
            print(f"  RSSI (best)  : {max(rssis)} dBm")
            print(f"  RSSI (worst) : {min(rssis)} dBm")
            print(f"  Measurements : {len(rssis)}")

        print("\n  Event Log (last 15):")
        for ev in self.events[-15:]:
            icon = {'CONNECTED': '✓', 'DISCONNECTED': '✗',
                    'RETRY': '↻', 'GOT_IP': '🌐', 'HEAP': '📊'}.get(
                        ev['type'], '•')
            print(f"    [{ev['time']}] {icon} {ev['type']:15s} | {ev['detail']}")
        print("=" * 60)

    def plot_rssi_timeline(self, filename="wifi_sta_rssi.png"):
        """Plot RSSI over time."""
        if not HAS_MATPLOTLIB or not self.rssi_history:
            return

        times = list(range(len(self.rssi_history)))
        rssis = [r['rssi'] for r in self.rssi_history]

        fig, ax = plt.subplots(figsize=(12, 5))
        ax.plot(times, rssis, 'b-o', markersize=4, linewidth=1.5, label='RSSI')

        # Threshold lines
        ax.axhline(y=-50, color='green', linestyle='--', alpha=0.5, label='Excellent (-50)')
        ax.axhline(y=-70, color='orange', linestyle='--', alpha=0.5, label='Fair (-70)')
        ax.axhline(y=-80, color='red', linestyle='--', alpha=0.5, label='Weak (-80)')

        # Fill color zones
        ax.fill_between(times, -30, -50, alpha=0.1, color='green')
        ax.fill_between(times, -50, -70, alpha=0.1, color='yellow')
        ax.fill_between(times, -70, -80, alpha=0.1, color='orange')
        ax.fill_between(times, -80, -100, alpha=0.1, color='red')

        ax.set_xlabel('Sample Number')
        ax.set_ylabel('RSSI (dBm)')
        ax.set_title('WiFi Station - RSSI Over Time')
        ax.legend(loc='lower left', fontsize=8)
        ax.set_ylim(-100, -20)
        ax.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig(filename, dpi=150)
        print(f"[OK] RSSI timeline saved: {filename}")
        plt.close()


def monitor_serial(port, baudrate, analyzer):
    """Monitor serial port secara real-time."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan.")
        sys.exit(1)

    print(f"[INFO] Connecting to {port} @ {baudrate}...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Connected. Monitoring... (Ctrl+C to stop)\n")

        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace')
                print(line, end='')
                result = analyzer.parse_line(line)

                if result in ("GOT_IP", "DISCONNECTED"):
                    analyzer.print_summary()

    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
        analyzer.print_summary()
        analyzer.plot_rssi_timeline()


def parse_from_file(filepath, analyzer):
    """Parse dari file."""
    print(f"[INFO] Reading: {filepath}")
    with open(filepath, 'r') as f:
        for line in f:
            analyzer.parse_line(line)
    analyzer.print_summary()
    analyzer.plot_rssi_timeline()


def main():
    parser = argparse.ArgumentParser(
        description='ESP32 WiFi Station - Debug & Analysis Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    args = parser.parse_args()

    analyzer = WiFiStationAnalyzer()

    if args.file:
        parse_from_file(args.file, analyzer)
    else:
        monitor_serial(args.port, args.baud, analyzer)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
============================================================================
Debug & Analysis Tool - ESP32_03_WiFi_Access_Point
============================================================================
Modul 13 - Network & IoT | Praktikum Sistem Embedded

Tool ini memonitor output serial dari ESP32 WiFi Access Point,
melacak client yang connect/disconnect, dan menampilkan
daftar client beserta statistik.

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

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARNING] pyserial not installed.")

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] matplotlib not installed.")


class APAnalyzer:
    """Analyzer untuk ESP32 Access Point monitoring."""

    def __init__(self):
        self.clients = {}           # MAC -> {connected, ip, rssi, connect_time}
        self.event_log = []         # List of events
        self.client_history = []    # Timestamped client count
        self.total_connections = 0
        self.total_disconnections = 0

        # Regex patterns
        self.mac_connect = re.compile(
            r'Client CONNECTED.*?MAC.*?:\s*([0-9a-fA-F:]{17})', re.DOTALL)
        self.mac_line = re.compile(r'MAC Address\s*:\s*([0-9a-fA-F:]{17})')
        self.mac_disconnect = re.compile(
            r'Client DISCONNECTED.*?MAC.*?:\s*([0-9a-fA-F:]{17})', re.DOTALL)
        self.ip_assign = re.compile(r'Client assigned IP:\s*([\d.]+)')
        self.client_info = re.compile(
            r'Client\s+(\d+):\s*MAC=([0-9a-fA-F:]{17})\s*\|\s*RSSI=(-?\d+)')
        self.connected_count = re.compile(r'Connected clients:\s*(\d+)')
        self.heap_pattern = re.compile(r'Free heap:\s*(\d+)')

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        timestamp = datetime.now().strftime("%H:%M:%S")

        # Client connected
        mac_match = self.mac_line.search(line)
        if mac_match and "CONNECTED" in line and "DISCONNECTED" not in line:
            mac = mac_match.group(1).lower()
            self.clients[mac] = {
                'connected': True, 'ip': None, 'rssi': 0,
                'connect_time': timestamp
            }
            self.total_connections += 1
            self.event_log.append({
                'time': timestamp, 'type': 'CONNECT', 'mac': mac
            })
            return "CONNECT"

        # Client disconnected
        if mac_match and "DISCONNECTED" in line:
            mac = mac_match.group(1).lower()
            if mac in self.clients:
                self.clients[mac]['connected'] = False
            self.total_disconnections += 1
            self.event_log.append({
                'time': timestamp, 'type': 'DISCONNECT', 'mac': mac
            })
            return "DISCONNECT"

        # IP assigned
        ip_match = self.ip_assign.search(line)
        if ip_match:
            ip = ip_match.group(1)
            # Assign IP ke client terbaru yang belum punya IP
            for mac, info in self.clients.items():
                if info['connected'] and info['ip'] is None:
                    info['ip'] = ip
                    break
            self.event_log.append({
                'time': timestamp, 'type': 'IP_ASSIGN', 'mac': '', 'ip': ip
            })
            return "IP_ASSIGN"

        # Periodic client info dari monitor task
        info_match = self.client_info.search(line)
        if info_match:
            mac = info_match.group(2).lower()
            rssi = int(info_match.group(3))
            if mac in self.clients:
                self.clients[mac]['rssi'] = rssi

        # Client count update
        count_match = self.connected_count.search(line)
        if count_match:
            count = int(count_match.group(1))
            self.client_history.append({
                'time': timestamp, 'count': count
            })

        return None

    def print_dashboard(self):
        """Tampilkan dashboard status AP."""
        connected_list = [(m, i) for m, i in self.clients.items() if i['connected']]
        disconnected_list = [(m, i) for m, i in self.clients.items() if not i['connected']]

        print("\n" + "=" * 65)
        print("  ESP32 ACCESS POINT - CLIENT DASHBOARD")
        print("=" * 65)
        print(f"  Total Connections    : {self.total_connections}")
        print(f"  Total Disconnections : {self.total_disconnections}")
        print(f"  Currently Connected  : {len(connected_list)}")
        print()

        if connected_list:
            print("  ┌──────────────────────┬────────────────┬───────┐")
            print("  │ MAC Address          │ IP Address     │ RSSI  │")
            print("  ├──────────────────────┼────────────────┼───────┤")
            for mac, info in connected_list:
                ip = info['ip'] or 'pending...'
                print(f"  │ {mac:20s} │ {ip:14s} │ {info['rssi']:4d}  │")
            print("  └──────────────────────┴────────────────┴───────┘")
        else:
            print("  [Tidak ada client yang terhubung]")

        if disconnected_list:
            print(f"\n  Previously connected ({len(disconnected_list)}):")
            for mac, info in disconnected_list:
                print(f"    - {mac} (IP was: {info.get('ip', 'N/A')})")

        # Event log terbaru
        print(f"\n  Recent Events (last 10):")
        for ev in self.event_log[-10:]:
            icon = '✓' if ev['type'] == 'CONNECT' else \
                   '✗' if ev['type'] == 'DISCONNECT' else '🌐'
            detail = ev.get('mac', '') or ev.get('ip', '')
            print(f"    [{ev['time']}] {icon} {ev['type']:12s} {detail}")
        print("=" * 65)

    def plot_client_history(self, filename="ap_clients.png"):
        """Plot jumlah client over time."""
        if not HAS_MATPLOTLIB or not self.client_history:
            return

        times = list(range(len(self.client_history)))
        counts = [c['count'] for c in self.client_history]

        fig, ax = plt.subplots(figsize=(10, 4))
        ax.step(times, counts, 'b-', linewidth=2, where='post')
        ax.fill_between(times, counts, alpha=0.3, step='post')
        ax.set_xlabel('Sample')
        ax.set_ylabel('Connected Clients')
        ax.set_title('ESP32 AP - Connected Clients Over Time')
        ax.set_ylim(bottom=0)
        ax.grid(True, alpha=0.3)
        ax.yaxis.set_major_locator(plt.MaxNLocator(integer=True))

        plt.tight_layout()
        plt.savefig(filename, dpi=150)
        print(f"[OK] Chart saved: {filename}")
        plt.close()


def monitor_serial(port, baudrate, analyzer):
    """Monitor serial secara real-time."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial required.")
        sys.exit(1)

    print(f"[INFO] Connecting to {port} @ {baudrate}...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Connected. Monitoring... (Ctrl+C to stop)\n")

        last_dashboard = time.time()
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace')
                print(line, end='')
                result = analyzer.parse_line(line)

                if result in ("CONNECT", "DISCONNECT", "IP_ASSIGN"):
                    analyzer.print_dashboard()

            # Periodic dashboard refresh setiap 30 detik
            if time.time() - last_dashboard > 30:
                analyzer.print_dashboard()
                last_dashboard = time.time()

    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Stopped.")
        analyzer.print_dashboard()
        analyzer.plot_client_history()


def parse_from_file(filepath, analyzer):
    """Parse dari file."""
    with open(filepath, 'r') as f:
        for line in f:
            analyzer.parse_line(line)
    analyzer.print_dashboard()
    analyzer.plot_client_history()


def main():
    parser = argparse.ArgumentParser(
        description='ESP32 WiFi AP - Debug & Analysis Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    args = parser.parse_args()

    analyzer = APAnalyzer()

    if args.file:
        parse_from_file(args.file, analyzer)
    else:
        monitor_serial(args.port, args.baud, analyzer)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
============================================================================
Debug & Analysis Tool - ESP32_01_WiFi_Scan
============================================================================
Modul 13 - Network & IoT | Praktikum Sistem Embedded

Tool ini membaca output serial dari ESP32 WiFi Scanner,
mem-parse data AP yang ditemukan, dan membuat visualisasi
berupa bar chart RSSI serta distribusi channel.

PENGGUNAAN:
  python3 debug_analysis.py                    # Default /dev/ttyUSB0
  python3 debug_analysis.py --port COM3        # Windows
  python3 debug_analysis.py --port /dev/ttyS0  # Custom port
  python3 debug_analysis.py --file output.txt  # Parse dari file

DEPENDENSI:
  pip install pyserial matplotlib
============================================================================
"""

import argparse
import re
import sys
import time
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
    matplotlib.use('Agg')  # Non-interactive backend
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] matplotlib not installed. Install: pip install matplotlib")


class WiFiScanAnalyzer:
    """Parser dan analyzer untuk output WiFi Scan ESP32."""

    def __init__(self):
        self.scan_results = []  # List of dicts: {ssid, rssi, channel, auth}
        self.scan_history = []  # List of scan_results per scan cycle
        # Regex pattern untuk parse baris tabel AP
        #  1 | MyHomeWiFi               |  -45 |  6 | WPA2_PSK   | Excellent
        self.ap_pattern = re.compile(
            r'\s*(\d+)\s*\|\s*(.+?)\s*\|\s*(-?\d+)\s*\|\s*(\d+)\s*\|\s*(\S+)\s*\|\s*(\S+)'
        )
        self.total_pattern = re.compile(r'Total AP ditemukan:\s*(\d+)')

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()

        # Deteksi awal scan baru
        if "WiFi Scan Results" in line:
            self.scan_results = []
            return "SCAN_START"

        # Parse baris AP
        match = self.ap_pattern.match(line)
        if match:
            ap_info = {
                'no': int(match.group(1)),
                'ssid': match.group(2).strip(),
                'rssi': int(match.group(3)),
                'channel': int(match.group(4)),
                'auth': match.group(5).strip(),
                'quality': match.group(6).strip(),
            }
            self.scan_results.append(ap_info)
            return "AP_FOUND"

        # Deteksi akhir scan
        total_match = self.total_pattern.search(line)
        if total_match:
            self.scan_history.append(list(self.scan_results))
            return "SCAN_END"

        return None

    def print_summary(self):
        """Tampilkan ringkasan hasil scan terakhir."""
        if not self.scan_results:
            print("[INFO] Belum ada data scan.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS WIFI SCAN")
        print("=" * 60)
        print(f"  Total AP terdeteksi : {len(self.scan_results)}")

        # Hitung distribusi auth mode
        auth_counts = defaultdict(int)
        channel_counts = defaultdict(int)
        for ap in self.scan_results:
            auth_counts[ap['auth']] += 1
            channel_counts[ap['channel']] += 1

        print("\n  Distribusi Auth Mode:")
        for auth, count in sorted(auth_counts.items()):
            print(f"    {auth:15s} : {count}")

        print("\n  Distribusi Channel:")
        for ch, count in sorted(channel_counts.items()):
            print(f"    Channel {ch:2d}      : {count}")

        # AP terkuat dan terlemah
        strongest = max(self.scan_results, key=lambda x: x['rssi'])
        weakest = min(self.scan_results, key=lambda x: x['rssi'])
        print(f"\n  Sinyal terkuat  : {strongest['ssid']} ({strongest['rssi']} dBm)")
        print(f"  Sinyal terlemah : {weakest['ssid']} ({weakest['rssi']} dBm)")
        print("=" * 60)

    def plot_rssi_chart(self, filename="wifi_scan_rssi.png"):
        """Buat bar chart RSSI dari hasil scan terakhir."""
        if not HAS_MATPLOTLIB:
            print("[SKIP] matplotlib tidak tersedia untuk plotting.")
            return
        if not self.scan_results:
            print("[INFO] Tidak ada data untuk di-plot.")
            return

        ssids = [ap['ssid'][:16] for ap in self.scan_results]
        rssis = [ap['rssi'] for ap in self.scan_results]
        # Warna berdasarkan kekuatan sinyal
        colors = []
        for r in rssis:
            if r >= -50:
                colors.append('#2ecc71')   # hijau - excellent
            elif r >= -60:
                colors.append('#27ae60')   # hijau tua - good
            elif r >= -70:
                colors.append('#f39c12')   # kuning - fair
            elif r >= -80:
                colors.append('#e74c3c')   # merah - weak
            else:
                colors.append('#c0392b')   # merah tua - very weak

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

        # Bar chart RSSI
        bars = ax1.barh(ssids, rssis, color=colors)
        ax1.set_xlabel('RSSI (dBm)')
        ax1.set_title('WiFi AP Signal Strength (RSSI)')
        ax1.axvline(x=-50, color='green', linestyle='--', alpha=0.5, label='Excellent')
        ax1.axvline(x=-70, color='orange', linestyle='--', alpha=0.5, label='Fair')
        ax1.axvline(x=-80, color='red', linestyle='--', alpha=0.5, label='Weak')
        ax1.legend(fontsize=8)
        ax1.invert_yaxis()

        # Pie chart distribusi channel
        channel_counts = defaultdict(int)
        for ap in self.scan_results:
            channel_counts[ap['channel']] += 1
        channels = [f"Ch {ch}" for ch in sorted(channel_counts.keys())]
        counts = [channel_counts[ch] for ch in sorted(channel_counts.keys())]
        ax2.pie(counts, labels=channels, autopct='%1.0f%%', startangle=90)
        ax2.set_title('Distribusi Channel WiFi')

        plt.tight_layout()
        plt.savefig(filename, dpi=150)
        print(f"[OK] Chart disimpan ke: {filename}")
        plt.close()


def monitor_serial(port, baudrate, analyzer):
    """Baca data serial secara real-time dan parse."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan untuk membaca serial port.")
        sys.exit(1)

    print(f"[INFO] Membuka serial port {port} @ {baudrate} baud...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Terhubung ke {port}")
        print("[INFO] Menunggu data scan... (Ctrl+C untuk berhenti)\n")

        scan_count = 0
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace')
                print(line, end='')  # Echo ke terminal

                result = analyzer.parse_line(line)
                if result == "SCAN_END":
                    scan_count += 1
                    print(f"\n[SCAN #{scan_count} selesai]")
                    analyzer.print_summary()
                    analyzer.plot_rssi_chart(f"wifi_scan_{scan_count}.png")

    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Monitoring dihentikan oleh user.")
        if analyzer.scan_results:
            analyzer.print_summary()
            analyzer.plot_rssi_chart()


def parse_from_file(filepath, analyzer):
    """Parse output dari file teks."""
    print(f"[INFO] Membaca file: {filepath}")
    with open(filepath, 'r') as f:
        for line in f:
            analyzer.parse_line(line)

    analyzer.print_summary()
    analyzer.plot_rssi_chart()


def main():
    parser = argparse.ArgumentParser(
        description='ESP32 WiFi Scan - Debug & Analysis Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--file', type=str, default=None,
                        help='Parse dari file teks (bukan serial)')
    args = parser.parse_args()

    analyzer = WiFiScanAnalyzer()

    if args.file:
        parse_from_file(args.file, analyzer)
    else:
        monitor_serial(args.port, args.baud, analyzer)


if __name__ == '__main__':
    main()

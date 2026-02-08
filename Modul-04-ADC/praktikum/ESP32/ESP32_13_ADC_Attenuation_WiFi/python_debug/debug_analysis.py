#!/usr/bin/env python3
"""
=============================================================================
Debug & Analysis Script — ESP32_13_ADC_Attenuation_WiFi
Modul 04 - ADC | Bonus: Fitur Khusus ESP32
=============================================================================

Script ini melakukan:
1. Monitoring serial output dari ESP32
2. Parsing data ADC (raw value, tegangan, atenuasi)
3. Visualisasi perbandingan antar atenuasi
4. Deteksi konflik ADC2+WiFi dari log

Cara penggunaan:
    python debug_analysis.py                    # Auto-detect port
    python debug_analysis.py --port /dev/ttyUSB0
    python debug_analysis.py --file output.log  # Analisis dari file log
=============================================================================
"""

import sys
import re
import time
import argparse
from datetime import datetime
from collections import defaultdict

# ==================== Konfigurasi ====================
BAUD_RATE = 115200
DEFAULT_PORTS = [
    '/dev/ttyUSB0', '/dev/ttyUSB1',
    '/dev/ttyACM0', '/dev/ttyACM1',
    'COM3', 'COM4', 'COM5',
]

# ==================== Parser Data ====================

class ADCDataParser:
    """Parser untuk output serial ESP32 ADC Attenuation demo"""

    def __init__(self):
        self.atten_data = {}       # {atenuasi: {'raw': val, 'mv': val}}
        self.monitoring_data = []   # List of {'raw': val, 'mv': val, 'rekomendasi': str}
        self.wifi_conflict = {
            'adc2_before_wifi': None,
            'adc2_during_wifi': None,
            'adc1_during_wifi': None,
            'adc2_after_wifi': None,
        }
        self.current_demo = ""
        self.log_lines = []

    def parse_line(self, line):
        """Parse satu baris output serial"""
        self.log_lines.append(line)

        # Deteksi demo aktif
        if "DEMO 1:" in line:
            self.current_demo = "attenuation"
        elif "DEMO 2:" in line:
            self.current_demo = "monitoring"
        elif "DEMO 3:" in line:
            self.current_demo = "wifi_conflict"

        # Parse data atenuasi (Demo 1)
        # Format: │ 0 dB     │ 0 - 750 mV       │      1234 │         456   │ OK          │
        atten_match = re.search(
            r'│\s*([\d.]+\s*dB)\s*│\s*[\d\s\-mV]+│\s*(\d+)\s*│\s*(\d+|no cali)',
            line
        )
        if atten_match:
            atten_name = atten_match.group(1).strip()
            raw = int(atten_match.group(2))
            mv_str = atten_match.group(3)
            mv = int(mv_str) if mv_str != 'no cali' else None
            self.atten_data[atten_name] = {'raw': raw, 'mv': mv}

        # Parse data monitoring (Demo 2)
        # Format: [ 1] Raw: 1234 |  456 mV | Rekomendasi atenuasi: 12dB
        mon_match = re.search(
            r'\[\s*\d+\]\s*Raw:\s*(\d+)\s*\|\s*(\d+)\s*mV\s*\|\s*Rekomendasi.*?:\s*(.+)',
            line
        )
        if mon_match:
            self.monitoring_data.append({
                'raw': int(mon_match.group(1)),
                'mv': int(mon_match.group(2)),
                'rekomendasi': mon_match.group(3).strip(),
            })

        # Parse konflik WiFi (Demo 3)
        if "ADC2" in line and "berhasil dibaca" in line and "kembali" not in line:
            raw_match = re.search(r'raw\s*=\s*(\d+)', line)
            if raw_match:
                if self.wifi_conflict['adc2_before_wifi'] is None:
                    self.wifi_conflict['adc2_before_wifi'] = int(raw_match.group(1))
                else:
                    self.wifi_conflict['adc2_during_wifi'] = int(raw_match.group(1))

        if "ADC1" in line and "TETAP berhasil" in line:
            raw_match = re.search(r'raw\s*=\s*(\d+)', line)
            if raw_match:
                self.wifi_conflict['adc1_during_wifi'] = int(raw_match.group(1))

        if "kembali berfungsi" in line:
            raw_match = re.search(r'raw\s*=\s*(\d+)', line)
            if raw_match:
                self.wifi_conflict['adc2_after_wifi'] = int(raw_match.group(1))

        if "GAGAL" in line and "ADC2" in line:
            self.wifi_conflict['adc2_during_wifi'] = "GAGAL"

    def print_report(self):
        """Cetak laporan analisis"""
        print("\n" + "=" * 70)
        print("  LAPORAN ANALISIS — ADC Attenuation & WiFi Conflict")
        print("=" * 70)

        # Laporan Atenuasi
        if self.atten_data:
            print("\n📊 PERBANDINGAN ATENUASI:")
            print("-" * 50)
            for atten, data in sorted(self.atten_data.items()):
                mv_str = f"{data['mv']} mV" if data['mv'] else "N/A"
                bar_len = data['raw'] * 30 // 4095 if data['raw'] else 0
                bar = "█" * bar_len + "░" * (30 - bar_len)
                print(f"  {atten:>8s}: Raw={data['raw']:4d} | {mv_str:>8s} | {bar}")

        # Laporan Monitoring
        if self.monitoring_data:
            print(f"\n📈 MONITORING ({len(self.monitoring_data)} sampel):")
            print("-" * 50)
            raws = [d['raw'] for d in self.monitoring_data]
            mvs = [d['mv'] for d in self.monitoring_data]
            print(f"  Raw: min={min(raws)}, max={max(raws)}, "
                  f"avg={sum(raws)//len(raws)}, noise={max(raws)-min(raws)} LSB")
            print(f"  mV:  min={min(mvs)}, max={max(mvs)}, "
                  f"avg={sum(mvs)//len(mvs)}")

        # Laporan WiFi Conflict
        print("\n⚠️  KONFLIK ADC2 + WiFi:")
        print("-" * 50)
        wc = self.wifi_conflict
        if wc['adc2_before_wifi'] is not None:
            print(f"  ADC2 sebelum WiFi : raw = {wc['adc2_before_wifi']} ✓")
        if wc['adc2_during_wifi'] == "GAGAL":
            print(f"  ADC2 saat WiFi    : GAGAL ✗ (konflik terkonfirmasi!)")
        elif wc['adc2_during_wifi'] is not None:
            print(f"  ADC2 saat WiFi    : raw = {wc['adc2_during_wifi']} (beruntung)")
        if wc['adc1_during_wifi'] is not None:
            print(f"  ADC1 saat WiFi    : raw = {wc['adc1_during_wifi']} ✓ (tidak terpengaruh)")
        if wc['adc2_after_wifi'] is not None:
            print(f"  ADC2 setelah WiFi : raw = {wc['adc2_after_wifi']} ✓ (pulih)")

        print("\n" + "=" * 70)


# ==================== Serial Monitor ====================

def find_serial_port():
    """Auto-detect serial port ESP32"""
    try:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        for port in ports:
            desc = (port.description or '').lower()
            if any(k in desc for k in ['cp210', 'ch340', 'ftdi', 'usb', 'uart']):
                print(f"  Port terdeteksi: {port.device} ({port.description})")
                return port.device
    except ImportError:
        pass

    for p in DEFAULT_PORTS:
        try:
            import serial
            s = serial.Serial(p, BAUD_RATE, timeout=0.1)
            s.close()
            return p
        except Exception:
            continue
    return None


def monitor_serial(port, duration=120):
    """Monitor serial output dan analisis data"""
    try:
        import serial
    except ImportError:
        print("ERROR: pyserial belum terinstall!")
        print("Install: pip install pyserial")
        sys.exit(1)

    parser = ADCDataParser()

    print(f"\n🔌 Menghubungkan ke {port} @ {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        print(f"ERROR: Tidak bisa buka {port}: {e}")
        sys.exit(1)

    print(f"✓ Terhubung! Monitoring selama {duration} detik...")
    print(f"  Tekan Ctrl+C untuk berhenti dan lihat laporan\n")
    print("-" * 70)

    start_time = time.time()
    try:
        while (time.time() - start_time) < duration:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                    print(f"[{timestamp}] {line}")
                    parser.parse_line(line)
    except KeyboardInterrupt:
        print("\n\n⏹ Monitoring dihentikan oleh user")
    finally:
        ser.close()

    parser.print_report()


def analyze_file(filepath):
    """Analisis dari file log yang sudah disimpan"""
    parser = ADCDataParser()

    print(f"\n📂 Membaca file: {filepath}")
    try:
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    parser.parse_line(line)
    except FileNotFoundError:
        print(f"ERROR: File tidak ditemukan: {filepath}")
        sys.exit(1)

    print(f"✓ {len(parser.log_lines)} baris diproses")
    parser.print_report()


# ==================== Main ====================

def main():
    argp = argparse.ArgumentParser(
        description='Debug & Analysis — ESP32 ADC Attenuation & WiFi Conflict'
    )
    argp.add_argument('--port', '-p', help='Serial port (misal /dev/ttyUSB0)')
    argp.add_argument('--file', '-f', help='Analisis dari file log')
    argp.add_argument('--duration', '-d', type=int, default=120,
                      help='Durasi monitoring dalam detik (default: 120)')
    args = argp.parse_args()

    print("=" * 70)
    print("  ESP32_13 ADC Attenuation & WiFi Conflict — Debug Analysis")
    print("=" * 70)

    if args.file:
        analyze_file(args.file)
    else:
        port = args.port
        if not port:
            print("\n🔍 Mencari port serial ESP32...")
            port = find_serial_port()
            if not port:
                print("ERROR: Tidak ada port serial ditemukan!")
                print("Gunakan: python debug_analysis.py --port /dev/ttyUSB0")
                sys.exit(1)

        monitor_serial(port, args.duration)


if __name__ == '__main__':
    main()

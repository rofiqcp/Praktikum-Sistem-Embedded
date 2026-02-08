#!/usr/bin/env python3
"""
=============================================================================
Debug & Analysis Script — ESP32_13_I2C_GPIO_Matrix
Modul 06 - I2C Sensor | Bonus: Fitur Khusus ESP32
=============================================================================

Script ini melakukan:
1. Monitoring serial output dari ESP32 I2C GPIO Matrix demo
2. Parsing hasil I2C scan dari setiap pin pair
3. Tracking runtime pin remapping events
4. Analisis dual bus scan results
5. Visualisasi perbandingan ESP32 vs STM32 pin flexibility

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

class I2CGPIOMatrixParser:
    """Parser untuk output serial ESP32 I2C GPIO Matrix demo"""

    def __init__(self):
        self.current_demo = ""
        self.pin_pairs_tested = []     # List of (sda, scl, success)
        self.scan_results = {}         # {pair_name: [list of (addr, device)]}
        self.remap_events = []         # List of remap events
        self.dual_bus_results = {
            'bus0_devices': 0,
            'bus1_devices': 0,
        }
        self.current_pair = ""
        self.current_scan = []
        self.log_lines = []

    def parse_line(self, line):
        """Parse satu baris output serial"""
        self.log_lines.append(line)

        # Deteksi demo
        if "DEMO 1:" in line:
            self.current_demo = "multi_pin"
        elif "DEMO 2:" in line:
            self.current_demo = "runtime_remap"
        elif "DEMO 3:" in line:
            self.current_demo = "dual_bus"
        elif "PERBANDINGAN" in line:
            self.current_demo = "comparison"

        # Parse pin pair info
        pair_match = re.search(
            r'Pin Pair\s*(\d+):\s*SDA=GPIO(\d+),\s*SCL=GPIO(\d+)', line
        )
        if pair_match:
            pair_num = int(pair_match.group(1))
            sda = int(pair_match.group(2))
            scl = int(pair_match.group(3))
            self.current_pair = f"Pair {pair_num}"
            self.current_scan = []

        # Parse bus creation success
        if "Bus I2C berhasil dibuat" in line:
            sda_scl_match = re.search(r'GPIO(\d+)/GPIO(\d+)', line)
            if sda_scl_match:
                self.pin_pairs_tested.append({
                    'sda': int(sda_scl_match.group(1)),
                    'scl': int(sda_scl_match.group(2)),
                    'success': True,
                })

        # Parse I2C aktif (remap events)
        if "I2C aktif di" in line:
            gpio_match = re.findall(r'GPIO(\d+)', line)
            if len(gpio_match) >= 2:
                self.remap_events.append({
                    'sda': int(gpio_match[0]),
                    'scl': int(gpio_match[1]),
                })

        # Parse scanned device
        # Format: │  60   │  0x3C    │ SSD1306 (OLED Display)            │
        device_match = re.search(
            r'│\s*(\d+)\s*│\s*0x([0-9A-Fa-f]+)\s*│\s*(.+?)\s*│', line
        )
        if device_match:
            addr = int(device_match.group(1))
            hex_addr = device_match.group(2)
            device = device_match.group(3).strip()
            self.current_scan.append({
                'addr': addr,
                'hex': hex_addr,
                'device': device,
            })

        # Parse total device count
        total_match = re.search(r'Total:\s*(\d+)\s*device', line)
        if total_match:
            if self.current_pair:
                self.scan_results[self.current_pair] = {
                    'count': int(total_match.group(1)),
                    'devices': list(self.current_scan),
                }

        # Parse tidak ada device
        if "Tidak ada device ditemukan" in line:
            if self.current_pair:
                self.scan_results[self.current_pair] = {
                    'count': 0,
                    'devices': [],
                }

        # Parse dual bus results
        bus_count_match = re.search(r'Bus\s*(\d+):\s*(\d+)\s*device', line)
        if bus_count_match and self.current_demo == "dual_bus":
            bus_num = int(bus_count_match.group(1))
            count = int(bus_count_match.group(2))
            if bus_num == 0:
                self.dual_bus_results['bus0_devices'] = count
            else:
                self.dual_bus_results['bus1_devices'] = count

        # Parse bus deleted/freed
        if "Bus dihapus" in line or "pin dibebaskan" in line:
            pass  # Track cleanup

    def print_report(self):
        """Cetak laporan analisis"""
        print("\n" + "=" * 70)
        print("  LAPORAN ANALISIS — I2C GPIO Matrix Pin Remapping")
        print("=" * 70)

        # Pin Pairs Tested
        if self.pin_pairs_tested:
            print(f"\n📌 PIN PAIRS YANG DI-TEST: {len(self.pin_pairs_tested)}")
            print("-" * 50)
            for i, pair in enumerate(self.pin_pairs_tested):
                status = "✓ Berhasil" if pair['success'] else "✗ Gagal"
                print(f"  Pair {i+1}: SDA=GPIO{pair['sda']}, "
                      f"SCL=GPIO{pair['scl']} — {status}")
            print(f"\n  Ini membuktikan ESP32 bisa I2C di pin MANA SAJA!")
            print(f"  STM32 hanya bisa di 2-4 pin pair fixed per I2C peripheral")

        # Scan Results
        if self.scan_results:
            print(f"\n🔍 HASIL I2C SCAN PER PIN PAIR:")
            print("-" * 50)
            for pair_name, result in self.scan_results.items():
                print(f"  {pair_name}: {result['count']} device ditemukan")
                for dev in result['devices']:
                    print(f"    └─ 0x{dev['hex']} : {dev['device']}")

        # Remap Events
        if self.remap_events:
            print(f"\n🔄 RUNTIME PIN REMAP EVENTS: {len(self.remap_events)}")
            print("-" * 50)
            for i, evt in enumerate(self.remap_events):
                print(f"  Remap {i+1}: SDA=GPIO{evt['sda']}, SCL=GPIO{evt['scl']}")
            print(f"\n  Bus I2C berpindah {len(self.remap_events)}x tanpa reboot!")
            print(f"  Ini TIDAK mungkin di STM32 (pin AF fixed)")

        # Dual Bus
        db = self.dual_bus_results
        if db['bus0_devices'] > 0 or db['bus1_devices'] > 0:
            print(f"\n🔀 DUAL BUS SIMULTAN:")
            print("-" * 50)
            print(f"  Bus 0: {db['bus0_devices']} device")
            print(f"  Bus 1: {db['bus1_devices']} device")
            print(f"  Total: {db['bus0_devices'] + db['bus1_devices']} device di 2 bus")
        else:
            print(f"\n🔀 DUAL BUS SIMULTAN:")
            print("-" * 50)
            print(f"  Tidak ada device terdeteksi (normal tanpa sensor)")
            print(f"  Hubungkan sensor I2C untuk melihat hasilnya")

        # ASCII art of GPIO matrix
        print(f"\n📊 VISUALISASI GPIO MATRIX ESP32:")
        print("-" * 50)
        print("  ┌─────────────┐     ┌────────┐")
        print("  │  I2C_NUM_0  │────▶│ Pin    │")
        print("  │  Controller │     │ Matrix │──▶ GPIO_X (SDA)")
        print("  │             │     │        │──▶ GPIO_Y (SCL)")
        print("  └─────────────┘     │        │")
        print("  ┌─────────────┐     │        │")
        print("  │  I2C_NUM_1  │────▶│  Remap │──▶ GPIO_A (SDA)")
        print("  │  Controller │     │  ke    │──▶ GPIO_B (SCL)")
        print("  │             │     │  pin   │")
        print("  └─────────────┘     │  mana  │")
        print("                      │  saja! │")
        print("                      └────────┘")
        print()
        print("  STM32 TIDAK memiliki matrix ini — pin fixed ke AF!")

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

    parser = I2CGPIOMatrixParser()

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
    """Analisis dari file log"""
    parser = I2CGPIOMatrixParser()

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
        description='Debug & Analysis — ESP32 I2C GPIO Matrix Pin Remapping'
    )
    argp.add_argument('--port', '-p', help='Serial port (misal /dev/ttyUSB0)')
    argp.add_argument('--file', '-f', help='Analisis dari file log')
    argp.add_argument('--duration', '-d', type=int, default=120,
                      help='Durasi monitoring dalam detik (default: 120)')
    args = argp.parse_args()

    print("=" * 70)
    print("  ESP32_13 I2C GPIO Matrix Pin Remapping — Debug Analysis")
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

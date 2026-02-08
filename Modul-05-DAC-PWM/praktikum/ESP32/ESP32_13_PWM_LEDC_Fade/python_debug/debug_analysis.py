#!/usr/bin/env python3
"""
=============================================================================
Debug & Analysis Script — ESP32_13_PWM_LEDC_Fade
Modul 05 - DAC/PWM | Bonus: Fitur Khusus ESP32
=============================================================================

Script ini melakukan:
1. Monitoring serial output dari ESP32 LEDC Fade demo
2. Parsing informasi fade (durasi, mode, channel)
3. Analisis perbandingan software vs hardware fade
4. Visualisasi timeline fade events

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

# ==================== Konfigurasi ====================
BAUD_RATE = 115200
DEFAULT_PORTS = [
    '/dev/ttyUSB0', '/dev/ttyUSB1',
    '/dev/ttyACM0', '/dev/ttyACM1',
    'COM3', 'COM4', 'COM5',
]

# ==================== Parser Data ====================

class LEDCFadeParser:
    """Parser untuk output serial ESP32 LEDC Fade demo"""

    def __init__(self):
        self.current_demo = ""
        self.fade_events = []      # List of fade event dicts
        self.sw_fade_info = {}     # Software fade timing
        self.hw_fade_info = {}     # Hardware fade timing
        self.gpio_pins = []        # Pin yang digunakan
        self.log_lines = []

    def parse_line(self, line):
        """Parse satu baris output serial"""
        self.log_lines.append(line)

        # Deteksi demo
        if "DEMO 1:" in line:
            self.current_demo = "linear"
        elif "DEMO 2:" in line:
            self.current_demo = "breathing"
        elif "DEMO 3:" in line:
            self.current_demo = "cascading"
        elif "DEMO 4:" in line:
            self.current_demo = "comparison"
        elif "DEMO 5:" in line:
            self.current_demo = "gpio_matrix"

        # Parse fade events
        if "Fade ON" in line and "selesai" in line:
            dur_match = re.search(r'durasi aktual:\s*(\d+)\s*ms', line)
            if dur_match:
                self.fade_events.append({
                    'demo': self.current_demo,
                    'type': 'fade_on',
                    'duration_ms': int(dur_match.group(1)),
                })

        if "Fade OFF" in line and "selesai" in line:
            self.fade_events.append({
                'demo': self.current_demo,
                'type': 'fade_off',
            })

        # Parse software vs hardware comparison
        if "Software fade selesai" in line:
            self.sw_fade_info['completed'] = True
        if "Hardware fade selesai" in line:
            self.hw_fade_info['completed'] = True

        sw_iter_match = re.search(r'Iterasi CPU:\s*(\d+)', line)
        if sw_iter_match:
            self.sw_fade_info['iterations'] = int(sw_iter_match.group(1))

        sw_time_match = re.search(r'Waktu:\s*(\d+)\s*ms', line)
        if sw_time_match:
            if 'iterations' not in self.sw_fade_info:
                # Ini software fade time
                self.sw_fade_info['time_ms'] = int(sw_time_match.group(1))
            elif 'time_ms' not in self.hw_fade_info:
                self.hw_fade_info['time_ms'] = int(sw_time_match.group(1))

        hw_free_match = re.search(r'CPU bebas melakukan\s*(\d+)', line)
        if hw_free_match:
            self.hw_fade_info['free_iterations'] = int(hw_free_match.group(1))

        # Parse GPIO info
        gpio_match = re.search(r'GPIO(\d+)', line)
        if gpio_match and "LED" in line:
            pin = int(gpio_match.group(1))
            if pin not in self.gpio_pins:
                self.gpio_pins.append(pin)

        # Parse Napas counting
        napas_match = re.search(r'Napas\s*(\d+)/(\d+)', line)
        if napas_match:
            pass  # Track breathing cycles

        # Parse cascading info
        if "NO_WAIT" in line and "CPU bebas" in line:
            self.fade_events.append({
                'demo': 'cascading',
                'type': 'no_wait_fade',
            })

    def print_report(self):
        """Cetak laporan analisis"""
        print("\n" + "=" * 70)
        print("  LAPORAN ANALISIS — LEDC Hardware Fade")
        print("=" * 70)

        # GPIO Matrix Info
        if self.gpio_pins:
            print(f"\n📌 GPIO PINS YANG DIGUNAKAN: {self.gpio_pins}")
            print("   (ESP32 bisa pakai pin mana saja — STM32 tidak bisa!)")

        # Fade Events Summary
        if self.fade_events:
            print(f"\n🔄 FADE EVENTS: {len(self.fade_events)} total")
            print("-" * 50)

            demo_counts = {}
            for evt in self.fade_events:
                d = evt['demo']
                demo_counts[d] = demo_counts.get(d, 0) + 1

            for demo, count in demo_counts.items():
                print(f"  {demo}: {count} fade events")

            # Tampilkan durasi fade yang tercatat
            timed = [e for e in self.fade_events if 'duration_ms' in e]
            if timed:
                durations = [e['duration_ms'] for e in timed]
                print(f"\n  Durasi fade tercatat: {durations} ms")
                print(f"  Rata-rata: {sum(durations)//len(durations)} ms")

        # Software vs Hardware Comparison
        if self.sw_fade_info or self.hw_fade_info:
            print("\n⚡ PERBANDINGAN SOFTWARE vs HARDWARE FADE:")
            print("-" * 50)

            if self.sw_fade_info:
                print("  Software Fade (STM32 style):")
                if 'iterations' in self.sw_fade_info:
                    print(f"    CPU iterations: {self.sw_fade_info['iterations']}")
                if 'time_ms' in self.sw_fade_info:
                    print(f"    Waktu: {self.sw_fade_info['time_ms']} ms")
                print("    CPU: SIBUK sepanjang fade ❌")

            if self.hw_fade_info:
                print("  Hardware Fade (ESP32 LEDC):")
                print("    CPU calls: 2 (set + start)")
                if 'time_ms' in self.hw_fade_info:
                    print(f"    Waktu: {self.hw_fade_info['time_ms']} ms")
                if 'free_iterations' in self.hw_fade_info:
                    print(f"    CPU bebas: {self.hw_fade_info['free_iterations']} iterasi ✓")
                print("    CPU: BEBAS selama fade ✅")

            # Hitung efisiensi
            if ('iterations' in self.sw_fade_info and
                    'free_iterations' in self.hw_fade_info):
                sw_iter = self.sw_fade_info['iterations']
                print(f"\n  📊 Efisiensi: HW fade membebaskan CPU dari {sw_iter} iterasi")
                print(f"     → CPU bisa mengerjakan hal lain saat LED fade!")

        # No-wait fade count
        no_wait = sum(1 for e in self.fade_events if e.get('type') == 'no_wait_fade')
        if no_wait > 0:
            print(f"\n🚀 CASCADING NO_WAIT FADES: {no_wait}")
            print("   Multiple fade berjalan PARALEL di hardware!")

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

    parser = LEDCFadeParser()

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
    parser = LEDCFadeParser()

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
        description='Debug & Analysis — ESP32 LEDC Hardware Fade'
    )
    argp.add_argument('--port', '-p', help='Serial port (misal /dev/ttyUSB0)')
    argp.add_argument('--file', '-f', help='Analisis dari file log')
    argp.add_argument('--duration', '-d', type=int, default=120,
                      help='Durasi monitoring dalam detik (default: 120)')
    args = argp.parse_args()

    print("=" * 70)
    print("  ESP32_13 PWM LEDC Hardware Fade — Debug Analysis")
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

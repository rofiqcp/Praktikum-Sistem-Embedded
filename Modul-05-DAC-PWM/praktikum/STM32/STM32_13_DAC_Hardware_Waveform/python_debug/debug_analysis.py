#!/usr/bin/env python3
"""
============================================================================
Debug Analysis Script - STM32_13: DAC Hardware Waveform Generator
============================================================================
Script ini menganalisis output serial dari program DAC Hardware Waveform
untuk memverifikasi software waveform generation dan membandingkan
performa CPU usage.

Penggunaan:
    python debug_analysis.py /dev/ttyUSB0
    python debug_analysis.py COM3
    python debug_analysis.py output.log
============================================================================
"""

import sys
import re
import time
from collections import defaultdict

def parse_line(line):
    """Parse satu baris output serial."""
    result = {}

    # Parse SW waveform output: [SW-TRIANGLE] val= 512 | updates/s=10000 | CPU~2.0%
    m = re.match(r'\[SW-(\w+)\]\s+val=\s*(\d+)\s+\|\s+updates/s=(\d+)\s+\|\s+CPU~([\d.]+)%', line)
    if m:
        result['type'] = 'sw_waveform'
        result['waveform'] = m.group(1)
        result['value'] = int(m.group(2))
        result['updates_per_sec'] = int(m.group(3))
        result['cpu_pct'] = float(m.group(4))
        return result

    # Parse HW DAC output: [HW-DAC] t=3s - Triangle aktif
    m = re.match(r'\[HW-DAC\]\s+t=(\d+)s\s+-\s+(.*)', line)
    if m:
        result['type'] = 'hw_dac'
        result['time'] = int(m.group(1))
        result['message'] = m.group(2).strip()
        return result

    # Parse continuous mode: [Triangle] val= 512
    m = re.match(r'\[(\w+)\]\s+val=\s*(\d+)', line)
    if m:
        result['type'] = 'continuous'
        result['waveform'] = m.group(1)
        result['value'] = int(m.group(2))
        return result

    # Parse mode switch: >> Beralih ke: Noise wave
    m = re.match(r'>>\s+Beralih ke:\s+(\w+)', line)
    if m:
        result['type'] = 'switch'
        result['waveform'] = m.group(1)
        return result

    return None

def analyze_data(lines):
    """Analisis data output serial."""
    sw_data = defaultdict(list)
    hw_data = []
    continuous_data = defaultdict(list)
    switches = []

    for line in lines:
        parsed = parse_line(line.strip())
        if parsed is None:
            continue
        if parsed['type'] == 'sw_waveform':
            sw_data[parsed['waveform']].append(parsed)
        elif parsed['type'] == 'hw_dac':
            hw_data.append(parsed)
        elif parsed['type'] == 'continuous':
            continuous_data[parsed['waveform']].append(parsed)
        elif parsed['type'] == 'switch':
            switches.append(parsed)

    print("\n" + "=" * 60)
    print("  ANALISIS DEBUG - DAC Hardware Waveform Generator")
    print("=" * 60)

    # Analisis Software Waveforms
    for wf_name, data_list in sw_data.items():
        print(f"\n--- Software {wf_name} ---")
        if data_list:
            values = [d['value'] for d in data_list]
            updates = [d['updates_per_sec'] for d in data_list]
            cpus = [d['cpu_pct'] for d in data_list]

            print(f"  Jumlah sampel      : {len(data_list)}")
            print(f"  Nilai min/max      : {min(values)} / {max(values)}")
            print(f"  Update rate avg    : {sum(updates)//len(updates)} updates/s")
            print(f"  CPU usage avg      : {sum(cpus)/len(cpus):.1f}%")

            # Verifikasi range
            if wf_name == 'TRIANGLE':
                if min(values) < 50 and max(values) > 900:
                    print(f"  [OK] Triangle sweep penuh terdeteksi")
                else:
                    print(f"  [WARN] Triangle sweep terbatas")
            elif wf_name == 'NOISE':
                unique_vals = len(set(values))
                print(f"  Nilai unik         : {unique_vals}")
                if unique_vals >= 3:
                    print(f"  [OK] Noise bervariasi")
                else:
                    print(f"  [WARN] Noise kurang bervariasi")
        else:
            print(f"  Tidak ada data.")

    # Analisis Hardware DAC (jika ada)
    if hw_data:
        print(f"\n--- Hardware DAC ---")
        print(f"  Events terdeteksi  : {len(hw_data)}")
        for d in hw_data:
            print(f"  t={d['time']}s: {d['message']}")
        print(f"  [OK] Hardware DAC aktif (CPU = 0%)")

    # Analisis Continuous Mode
    if continuous_data:
        print(f"\n--- Mode Kontinu ---")
        for wf_name, data_list in continuous_data.items():
            values = [d['value'] for d in data_list]
            print(f"  {wf_name}: {len(data_list)} sampel, range={min(values)}-{max(values)}")

    # Ringkasan
    print("\n" + "=" * 60)
    print("  VERIFIKASI")
    print("=" * 60)

    errors = 0
    if sw_data:
        print(f"  [OK] Software waveform berfungsi ({len(sw_data)} jenis)")
    else:
        print(f"  [GAGAL] Tidak ada data software waveform")
        errors += 1

    if hw_data:
        print(f"  [OK] Hardware DAC terdeteksi")
    else:
        print(f"  [INFO] Hardware DAC tidak terdeteksi (normal untuk F103/F401/F411)")

    if switches:
        print(f"  [OK] Mode switching berfungsi ({len(switches)} switch)")

    if errors == 0:
        print(f"\n  >>> SEMUA TES BERHASIL <<<")
    else:
        print(f"\n  >>> {errors} TES GAGAL <<<")
    print("=" * 60)

def main():
    if len(sys.argv) < 2:
        print("Penggunaan: python debug_analysis.py <serial_port_or_logfile>")
        print("Contoh    : python debug_analysis.py /dev/ttyUSB0")
        print("            python debug_analysis.py output.log")
        sys.exit(1)

    source = sys.argv[1]

    try:
        with open(source, 'r') as f:
            lines = f.readlines()
        print(f"Membaca dari file: {source}")
        analyze_data(lines)
    except FileNotFoundError:
        try:
            import serial
            print(f"Membuka serial port: {source} (115200 baud)")
            print("Tekan Ctrl+C untuk berhenti dan melihat analisis...\n")

            ser = serial.Serial(source, 115200, timeout=1)
            lines = []
            try:
                while True:
                    line = ser.readline().decode('utf-8', errors='ignore')
                    if line:
                        print(line, end='')
                        lines.append(line)
            except KeyboardInterrupt:
                pass
            finally:
                ser.close()

            analyze_data(lines)
        except ImportError:
            print("Error: pyserial belum terinstall.")
            print("Install: pip install pyserial")
            sys.exit(1)
        except Exception as e:
            print(f"Error membuka serial port: {e}")
            sys.exit(1)

if __name__ == '__main__':
    main()

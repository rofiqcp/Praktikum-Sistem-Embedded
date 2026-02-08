#!/usr/bin/env python3
"""
============================================================================
Debug Analysis Script - STM32_13: ADC Analog Watchdog
============================================================================
Script ini menganalisis output serial dari program ADC Analog Watchdog
untuk memverifikasi respons hardware AWD dan membandingkan dengan
software polling.

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
    
    # Parse HW-AWD output
    m = re.match(r'\[HW-AWD\]\s+ADC=\s*(\d+)\s+\(([\d.]+)V\)\s+(.*?)\|\s+AWD alerts=(\d+)', line)
    if m:
        result['type'] = 'hw_awd'
        result['adc'] = int(m.group(1))
        result['voltage'] = float(m.group(2))
        result['status'] = m.group(3).strip()
        result['alerts'] = int(m.group(4))
        return result
    
    # Parse SW-POLL output
    m = re.match(r'\[SW-POLL\]\s+ADC=\s*(\d+)\s+\(([\d.]+)V\)\s+(.*?)\|\s+deteksi=(\d+)\s+\|\s+avg=([\d.]+)us', line)
    if m:
        result['type'] = 'sw_poll'
        result['adc'] = int(m.group(1))
        result['voltage'] = float(m.group(2))
        result['status'] = m.group(3).strip()
        result['detections'] = int(m.group(4))
        result['avg_us'] = float(m.group(5))
        return result
    
    # Parse AWD interrupt
    m = re.match(r'.*AWD INTERRUPT.*Nilai=(\d+)\s+\(([\d.]+)V\)\s+pada\s+t=(\d+)ms', line)
    if m:
        result['type'] = 'awd_interrupt'
        result['adc'] = int(m.group(1))
        result['voltage'] = float(m.group(2))
        result['timestamp'] = int(m.group(3))
        return result
    
    # Parse AWD alert dalam monitoring kontinu
    m = re.match(r'.*AWD ALERT.*val=(\d+)\s+t=(\d+)ms', line)
    if m:
        result['type'] = 'awd_alert'
        result['adc'] = int(m.group(1))
        result['timestamp'] = int(m.group(2))
        return result
    
    return None

def analyze_data(lines):
    """Analisis data dari output serial."""
    hw_awd_data = []
    sw_poll_data = []
    awd_interrupts = []
    awd_alerts = []
    
    for line in lines:
        parsed = parse_line(line.strip())
        if parsed is None:
            continue
        if parsed['type'] == 'hw_awd':
            hw_awd_data.append(parsed)
        elif parsed['type'] == 'sw_poll':
            sw_poll_data.append(parsed)
        elif parsed['type'] == 'awd_interrupt':
            awd_interrupts.append(parsed)
        elif parsed['type'] == 'awd_alert':
            awd_alerts.append(parsed)
    
    print("\n" + "=" * 60)
    print("  ANALISIS DEBUG - ADC Analog Watchdog")
    print("=" * 60)
    
    # Analisis Hardware AWD
    print("\n--- Hardware AWD ---")
    if hw_awd_data:
        adc_values = [d['adc'] for d in hw_awd_data]
        voltages = [d['voltage'] for d in hw_awd_data]
        print(f"  Jumlah sampel     : {len(hw_awd_data)}")
        print(f"  ADC min/max       : {min(adc_values)} / {max(adc_values)}")
        print(f"  Tegangan min/max  : {min(voltages):.2f}V / {max(voltages):.2f}V")
        print(f"  Total AWD alerts  : {hw_awd_data[-1]['alerts'] if hw_awd_data else 0}")
    else:
        print("  Tidak ada data HW-AWD.")
    
    # Analisis AWD Interrupts
    print(f"\n--- AWD Interrupts ---")
    print(f"  Total interrupts  : {len(awd_interrupts)}")
    if len(awd_interrupts) >= 2:
        timestamps = [d['timestamp'] for d in awd_interrupts]
        intervals = [timestamps[i+1] - timestamps[i] for i in range(len(timestamps)-1)]
        if intervals:
            print(f"  Interval min/max  : {min(intervals)}ms / {max(intervals)}ms")
            print(f"  Interval rata-rata: {sum(intervals)/len(intervals):.1f}ms")
    
    for i, intr in enumerate(awd_interrupts[:5]):
        print(f"  [{i+1}] ADC={intr['adc']} ({intr['voltage']:.2f}V) t={intr['timestamp']}ms")
    if len(awd_interrupts) > 5:
        print(f"  ... dan {len(awd_interrupts)-5} interrupt lainnya")
    
    # Analisis Software Polling
    print(f"\n--- Software Polling ---")
    if sw_poll_data:
        avg_times = [d['avg_us'] for d in sw_poll_data]
        print(f"  Jumlah sampel     : {len(sw_poll_data)}")
        print(f"  Waktu poll avg    : {sum(avg_times)/len(avg_times):.1f} us")
        print(f"  Total deteksi SW  : {sw_poll_data[-1]['detections'] if sw_poll_data else 0}")
    else:
        print("  Tidak ada data SW-POLL.")
    
    # Perbandingan
    print(f"\n--- Perbandingan ---")
    print(f"  HW AWD response   : < 1 us (hardware interrupt)")
    if sw_poll_data:
        avg_poll = sum(avg_times) / len(avg_times)
        print(f"  SW Poll response  : ~{avg_poll:.0f} us (software polling)")
        print(f"  Speedup HW vs SW  : ~{avg_poll:.0f}x lebih cepat")
    
    print("\n" + "=" * 60)
    print("  VERIFIKASI")
    print("=" * 60)
    
    errors = 0
    if not hw_awd_data:
        print("  [GAGAL] Tidak ada data Hardware AWD")
        errors += 1
    else:
        print("  [OK] Hardware AWD berfungsi")
    
    if not awd_interrupts and not awd_alerts:
        print("  [INFO] Tidak ada AWD interrupt terdeteksi")
        print("         (Normal jika potentiometer dalam window aman)")
    else:
        print(f"  [OK] AWD interrupt terdeteksi ({len(awd_interrupts) + len(awd_alerts)} total)")
    
    if not sw_poll_data:
        print("  [GAGAL] Tidak ada data Software Polling")
        errors += 1
    else:
        print("  [OK] Software Polling berfungsi")
    
    if errors == 0:
        print("\n  >>> SEMUA TES BERHASIL <<<")
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
    
    # Cek apakah input adalah file log
    try:
        with open(source, 'r') as f:
            lines = f.readlines()
        print(f"Membaca dari file: {source}")
        analyze_data(lines)
    except FileNotFoundError:
        # Coba sebagai serial port
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

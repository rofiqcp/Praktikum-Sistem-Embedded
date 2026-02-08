"""
Debug & Analysis Tool - ESP32_09_Dynamic_Frequency
Performance chart dan frequency vs throughput plot.

Penggunaan: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import re
import json
import time
from datetime import datetime
from collections import defaultdict

# Konfigurasi
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD_RATE = 115200
LOG_FILE = "dynamic_frequency_log.json"

def parse_serial_line(line):
    """Parse satu baris output serial ESP32."""
    data = {"timestamp": datetime.now().isoformat(), "raw": line}
    
    # Parse frekuensi aktual
    m = re.search(r"Frekuensi aktual:\s*(\d+)\s*MHz", line)
    if m:
        data["actual_freq"] = int(m.group(1))
    
    # Parse hasil benchmark
    m = re.search(r"Waktu:\s*(\d+)\s*us.*Iterasi:\s*(\d+).*MIPS:\s*([\d.]+)", line)
    if m:
        data["time_us"] = int(m.group(1))
        data["iterations"] = int(m.group(2))
        data["mips"] = float(m.group(3))
    
    # Parse baris tabel
    m = re.search(r"(\d+)\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)", line)
    if m:
        data["table_entry"] = {
            "freq_mhz": int(m.group(1)),
            "time_ms": float(m.group(2)),
            "mips": float(m.group(3)),
            "current_ma": float(m.group(4)),
            "efficiency": float(m.group(5))
        }
    
    return data

def monitor_frequency():
    """Monitor frequency scaling dan kumpulkan data."""
    print(f"=== Dynamic Frequency Monitor ===")
    print(f"Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f"Log file: {LOG_FILE}")
    print("Tekan Ctrl+C untuk berhenti\n")
    
    freq_results = []
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode("utf-8", errors="replace").strip()
                except Exception:
                    continue
                
                if not line:
                    continue
                
                print(f"  {line}")
                parsed = parse_serial_line(line)
                
                if "table_entry" in parsed:
                    freq_results.append(parsed["table_entry"])
                    
    except serial.SerialException as e:
        print(f"\nError serial: {e}")
    except KeyboardInterrupt:
        print("\n\nMonitoring dihentikan.")
    finally:
        # Simpan log
        log_data = {
            "session": datetime.now().isoformat(),
            "frequency_results": freq_results
        }
        with open(LOG_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Data tersimpan ke {LOG_FILE}")
        
        print_performance_chart(freq_results)

def print_performance_chart(results):
    """Tampilkan chart performa ASCII."""
    print("\n" + "=" * 70)
    print("  PERFORMANCE vs POWER CHART")
    print("=" * 70)
    
    if not results:
        print("Tidak ada data untuk dianalisis.")
        return
    
    # Sort berdasarkan frekuensi
    results.sort(key=lambda x: x["freq_mhz"])
    
    # Chart MIPS vs Frequency
    print("\n  MIPS vs Frekuensi (throughput):")
    max_mips = max(r["mips"] for r in results) if results else 1
    for r in reversed(results):
        bar_len = int(r["mips"] / max_mips * 40) if max_mips > 0 else 0
        bar = "█" * bar_len
        print(f"  {r['freq_mhz']:4d} MHz: {bar} {r['mips']:.2f}")
    
    # Chart Arus vs Frequency
    print(f"\n  Arus (mA) vs Frekuensi (konsumsi daya):")
    max_current = max(r["current_ma"] for r in results) if results else 1
    for r in reversed(results):
        bar_len = int(r["current_ma"] / max_current * 40) if max_current > 0 else 0
        bar = "▓" * bar_len
        print(f"  {r['freq_mhz']:4d} MHz: {bar} {r['current_ma']:.1f} mA")
    
    # Chart Efisiensi
    print(f"\n  Efisiensi (MIPS/mA) vs Frekuensi:")
    max_eff = max(r["efficiency"] for r in results) if results else 1
    for r in reversed(results):
        bar_len = int(r["efficiency"] / max_eff * 40) if max_eff > 0 else 0
        bar = "░" * bar_len
        print(f"  {r['freq_mhz']:4d} MHz: {bar} {r['efficiency']:.4f}")
    
    # Waktu eksekusi
    print(f"\n  Waktu Benchmark (ms):")
    max_time = max(r["time_ms"] for r in results) if results else 1
    for r in reversed(results):
        bar_len = int(r["time_ms"] / max_time * 40) if max_time > 0 else 0
        bar = "▒" * bar_len
        print(f"  {r['freq_mhz']:4d} MHz: {bar} {r['time_ms']:.1f} ms")
    
    # Ringkasan
    fastest = min(results, key=lambda x: x["time_ms"])
    most_efficient = max(results, key=lambda x: x["efficiency"])
    lowest_power = min(results, key=lambda x: x["current_ma"])
    
    print(f"\n  Ringkasan:")
    print(f"    Tercepat       : {fastest['freq_mhz']} MHz ({fastest['time_ms']:.1f} ms)")
    print(f"    Paling efisien : {most_efficient['freq_mhz']} MHz ({most_efficient['efficiency']:.4f} MIPS/mA)")
    print(f"    Daya terendah  : {lowest_power['freq_mhz']} MHz ({lowest_power['current_ma']:.1f} mA)")
    
    # Estimasi battery life untuk tiap frekuensi
    battery_mah = 2000
    print(f"\n  Estimasi Battery Life ({battery_mah} mAh LiPo):")
    for r in reversed(results):
        hours = battery_mah / r["current_ma"]
        print(f"    {r['freq_mhz']:4d} MHz: {hours:.1f} jam ({hours/24:.1f} hari)")
    
    print("\n" + "=" * 70)

if __name__ == "__main__":
    monitor_frequency()

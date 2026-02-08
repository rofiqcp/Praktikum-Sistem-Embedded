"""
Debug & Analysis Tool - ESP32_11_Battery_Powered_Logger
Battery discharge curve, data logger dashboard, estimated runtime calculator.

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
LOG_FILE = "battery_logger_log.json"

def parse_serial_line(line):
    """Parse satu baris output serial ESP32."""
    data = {"timestamp": datetime.now().isoformat(), "raw": line}
    
    # Parse DATA CSV
    m = re.search(r"DATA,(\d+),(\d+),(\d+),(\d+),(\d+)", line)
    if m:
        data["data_entry"] = {
            "log_num": int(m.group(1)),
            "battery_mv": int(m.group(2)),
            "battery_pct": int(m.group(3)),
            "sensor_raw": int(m.group(4)),
            "timestamp_us": int(m.group(5))
        }
    
    # Parse boot count
    m = re.search(r"Boot ke-(\d+)", line)
    if m:
        data["boot_count"] = int(m.group(1))
    
    # Parse tegangan
    m = re.search(r"Tegangan:\s*(\d+)\s*mV", line)
    if m:
        data["battery_mv"] = int(m.group(1))
    
    # Parse persentase baterai
    m = re.search(r"\]\s*(\d+)%", line)
    if m:
        data["battery_pct"] = int(m.group(1))
    
    # Parse sensor
    m = re.search(r"Sensor RAW\s*:\s*(\d+)", line)
    if m:
        data["sensor_raw"] = int(m.group(1))
    
    # Parse durasi siklus
    m = re.search(r"Durasi siklus.*?(\d+)\s*us", line)
    if m:
        data["cycle_duration_us"] = int(m.group(1))
    
    # Parse duty cycle
    m = re.search(r"Duty cycle\s*:\s*([\d.]+)%", line)
    if m:
        data["duty_cycle"] = float(m.group(1))
    
    # Parse peringatan baterai
    if "BATERAI KRITIS" in line or "Baterai rendah" in line:
        data["low_battery_warning"] = True
    
    return data

def monitor_logger():
    """Monitor battery-powered logger."""
    print(f"=== Battery-Powered Logger Monitor ===")
    print(f"Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f"Log file: {LOG_FILE}")
    print("Tekan Ctrl+C untuk berhenti\n")
    
    data_entries = []
    battery_readings = []
    cycle_durations = []
    
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
                
                # Warna berdasarkan konten
                if "KRITIS" in line or "HIBERNASI" in line:
                    print(f"  \033[91m{line}\033[0m")
                elif "RENDAH" in line or "PERINGATAN" in line:
                    print(f"  \033[93m{line}\033[0m")
                elif "DATA," in line:
                    print(f"  \033[92m{line}\033[0m")
                else:
                    print(f"  {line}")
                
                parsed = parse_serial_line(line)
                
                if "data_entry" in parsed:
                    data_entries.append(parsed["data_entry"])
                
                if "battery_mv" in parsed:
                    battery_readings.append({
                        "time": parsed["timestamp"],
                        "mv": parsed["battery_mv"]
                    })
                
                if "cycle_duration_us" in parsed:
                    cycle_durations.append(parsed["cycle_duration_us"])
                    
    except serial.SerialException as e:
        print(f"\nError serial: {e}")
    except KeyboardInterrupt:
        print("\n\nMonitoring dihentikan.")
    finally:
        log_data = {
            "session": datetime.now().isoformat(),
            "data_entries": data_entries,
            "battery_readings": battery_readings,
            "cycle_durations": cycle_durations
        }
        with open(LOG_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Data tersimpan ke {LOG_FILE}")
        
        print_dashboard(data_entries, battery_readings, cycle_durations)

def print_dashboard(entries, battery, cycles):
    """Tampilkan logger dashboard lengkap."""
    print("\n" + "=" * 65)
    print("  BATTERY-POWERED DATA LOGGER DASHBOARD")
    print("=" * 65)
    
    if not entries:
        print("Tidak ada data entry untuk dianalisis.")
        return
    
    # Battery discharge curve (ASCII)
    print(f"\n  Battery Discharge Curve ({len(entries)} readings):")
    battery_mvs = [e["battery_mv"] for e in entries]
    min_v = min(battery_mvs) if battery_mvs else 3300
    max_v = max(battery_mvs) if battery_mvs else 4200
    
    # Normalize dan plot
    height = 10
    width = min(50, len(battery_mvs))
    step = max(1, len(battery_mvs) // width)
    sampled = battery_mvs[::step][:width]
    
    v_range = max_v - min_v if max_v > min_v else 1
    
    for row in range(height, -1, -1):
        v_level = min_v + (v_range * row / height)
        line_chars = []
        for val in sampled:
            normalized = (val - min_v) / v_range * height
            if int(normalized) >= row:
                line_chars.append("█")
            else:
                line_chars.append(" ")
        label = f"{v_level:.0f}mV"
        print(f"  {label:>7s} |{''.join(line_chars)}|")
    
    print(f"  {'':>7s} +{'─' * len(sampled)}+")
    print(f"  {'':>7s}  {'1':>{1}}{'':>{len(sampled)-2}}{len(entries)}")
    print(f"  {'':>7s}  {'Waktu (log entries)':^{len(sampled)}}")
    
    # Statistik baterai
    print(f"\n  Statistik Baterai:")
    print(f"    Awal     : {battery_mvs[0]} mV ({entries[0]['battery_pct']}%)")
    print(f"    Akhir    : {battery_mvs[-1]} mV ({entries[-1]['battery_pct']}%)")
    print(f"    Min      : {min(battery_mvs)} mV")
    print(f"    Max      : {max(battery_mvs)} mV")
    
    drop = battery_mvs[0] - battery_mvs[-1]
    if len(entries) > 1 and drop > 0:
        rate_per_entry = drop / (len(entries) - 1)
        entries_to_empty = (battery_mvs[-1] - 3300) / rate_per_entry if rate_per_entry > 0 else float('inf')
        print(f"    Penurunan: {drop} mV total ({rate_per_entry:.1f} mV/siklus)")
        print(f"    Estimasi sisa: {entries_to_empty:.0f} siklus")
    
    # Sensor data
    sensor_vals = [e["sensor_raw"] for e in entries]
    print(f"\n  Statistik Sensor:")
    print(f"    Min      : {min(sensor_vals)}")
    print(f"    Max      : {max(sensor_vals)}")
    print(f"    Rata-rata: {sum(sensor_vals)/len(sensor_vals):.1f}")
    
    # Cycle duration
    if cycles:
        print(f"\n  Durasi Siklus Aktif:")
        print(f"    Min      : {min(cycles)/1000:.1f} ms")
        print(f"    Max      : {max(cycles)/1000:.1f} ms")
        print(f"    Rata-rata: {sum(cycles)/len(cycles)/1000:.1f} ms")
    
    # Runtime estimation
    print(f"\n  Estimasi Runtime:")
    active_current = 50.0   # mA saat aktif
    sleep_current = 0.01    # mA saat deep sleep
    sleep_sec = 60
    active_ms = sum(cycles) / len(cycles) / 1000 if cycles else 500
    active_sec = active_ms / 1000
    
    duty = active_sec / (active_sec + sleep_sec)
    avg_current = (active_current * duty) + (sleep_current * (1 - duty))
    
    batteries = [
        ("LiPo 500mAh", 500),
        ("LiPo 1000mAh", 1000),
        ("LiPo 2000mAh", 2000),
        ("18650 3000mAh", 3000),
    ]
    
    print(f"    Duty cycle       : {duty*100:.3f}%")
    print(f"    Arus rata-rata   : {avg_current:.3f} mA")
    print(f"    Waktu aktif/siklus: {active_ms:.1f} ms")
    
    for name, cap in batteries:
        hours = cap / avg_current if avg_current > 0 else 0
        print(f"    {name:16s}: {hours:.0f} jam ({hours/24:.0f} hari, {hours/24/30:.1f} bulan)")
    
    print("\n" + "=" * 65)

if __name__ == "__main__":
    monitor_logger()

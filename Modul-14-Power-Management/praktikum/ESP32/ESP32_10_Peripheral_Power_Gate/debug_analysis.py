"""
Debug & Analysis Tool - ESP32_10_Peripheral_Power_Gate
Peripheral power budget calculator dan pie chart distribusi daya.

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
LOG_FILE = "peripheral_power_log.json"

def parse_serial_line(line):
    """Parse satu baris output serial ESP32."""
    data = {"timestamp": datetime.now().isoformat(), "raw": line}
    
    # Parse peripheral ON
    m = re.search(r"\[ON\]\s+([\w\s()]+)\s+\(\+([\d.]+)\s*mA\)", line)
    if m:
        data["event"] = "enable"
        data["peripheral"] = m.group(1).strip()
        data["current_ma"] = float(m.group(2))
    
    # Parse peripheral OFF
    m = re.search(r"\[OFF\]\s+([\w\s()]+)\s+\(hemat\s+([\d.]+)\s*mA.*total hemat:\s*([\d.]+)", line)
    if m:
        data["event"] = "disable"
        data["peripheral"] = m.group(1).strip()
        data["saved_ma"] = float(m.group(2))
        data["total_saved_ma"] = float(m.group(3))
    
    # Parse status tabel
    m = re.search(r"([\w\s()]+)\s*\|\s*(AKTIF|OFF)\s*\|\s*([\d.]+)\s*mA", line)
    if m:
        data["status_entry"] = {
            "name": m.group(1).strip(),
            "enabled": m.group(2) == "AKTIF",
            "current_ma": float(m.group(3))
        }
    
    # Parse power budget
    m = re.search(r"TOTAL\s*\|\s*([\d.]+)", line)
    if m:
        data["total_current"] = float(m.group(1))
    
    # Parse battery life
    m = re.search(r"Semua peripheral ON\s*:\s*([\d.]+)\s*jam", line)
    if m:
        data["battery_all_on"] = float(m.group(1))
    
    m = re.search(r"Peripheral dioptimasi:\s*([\d.]+)\s*jam", line)
    if m:
        data["battery_optimized"] = float(m.group(1))
    
    return data

def monitor_peripherals():
    """Monitor peripheral power gating."""
    print(f"=== Peripheral Power Gate Monitor ===")
    print(f"Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f"Log file: {LOG_FILE}")
    print("Tekan Ctrl+C untuk berhenti\n")
    
    events = []
    status_entries = []
    
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
                
                # Warna berdasarkan event
                if "[ON]" in line:
                    print(f"  \033[92m{line}\033[0m")
                elif "[OFF]" in line:
                    print(f"  \033[91m{line}\033[0m")
                elif "TOTAL" in line:
                    print(f"  \033[93m{line}\033[0m")
                else:
                    print(f"  {line}")
                
                parsed = parse_serial_line(line)
                
                if "event" in parsed:
                    events.append(parsed)
                if "status_entry" in parsed:
                    status_entries.append(parsed["status_entry"])
                    
    except serial.SerialException as e:
        print(f"\nError serial: {e}")
    except KeyboardInterrupt:
        print("\n\nMonitoring dihentikan.")
    finally:
        log_data = {
            "session": datetime.now().isoformat(),
            "events": events,
            "status_entries": status_entries
        }
        with open(LOG_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Data tersimpan ke {LOG_FILE}")
        
        print_power_budget(events, status_entries)

def print_power_budget(events, status_entries):
    """Tampilkan power budget calculator dan distribusi."""
    print("\n" + "=" * 60)
    print("  PERIPHERAL POWER BUDGET CALCULATOR")
    print("=" * 60)
    
    # Kumpulkan peripheral unik
    peripherals = {}
    for entry in status_entries:
        name = entry["name"]
        if name not in peripherals:
            peripherals[name] = entry
    
    if not peripherals:
        print("Tidak ada data peripheral untuk dianalisis.")
        return
    
    # Hitung total
    total_all = sum(p["current_ma"] for p in peripherals.values())
    total_active = sum(p["current_ma"] for p in peripherals.values() if p["enabled"])
    total_saved = total_all - total_active
    
    # Pie chart ASCII
    print(f"\n  Distribusi Daya Peripheral (Total: {total_all:.1f} mA):")
    print(f"  {'─' * 50}")
    
    for name, info in sorted(peripherals.items(), key=lambda x: x[1]["current_ma"], reverse=True):
        pct = info["current_ma"] / total_all * 100 if total_all > 0 else 0
        bar_len = int(pct / 2)
        status = "ON " if info["enabled"] else "OFF"
        bar_char = "█" if info["enabled"] else "░"
        bar = bar_char * bar_len
        print(f"  [{status}] {name:16s}: {bar:25s} {info['current_ma']:.1f} mA ({pct:.1f}%)")
    
    print(f"  {'─' * 50}")
    print(f"  Total semua   : {total_all:.1f} mA")
    print(f"  Total aktif   : {total_active:.1f} mA")
    print(f"  Total hemat   : {total_saved:.1f} mA ({total_saved/total_all*100:.1f}% penghematan)")
    
    # Power budget dengan komponen sistem
    print(f"\n  Power Budget Lengkap (dengan sistem):")
    sys_components = {
        "CPU (80MHz)": 30.0,
        "Flash": 8.0,
        "Sistem Dasar": 5.0,
        "Peripheral Aktif": total_active
    }
    
    grand_total = sum(sys_components.values())
    
    for name, current in sys_components.items():
        pct = current / grand_total * 100
        bar = "█" * int(pct / 2)
        print(f"    {name:20s}: {bar:25s} {current:.1f} mA ({pct:.1f}%)")
    print(f"    {'─' * 50}")
    print(f"    GRAND TOTAL       : {grand_total:.1f} mA")
    
    # Battery life estimation
    batteries = [
        ("CR2032 (220mAh)", 220),
        ("AAA (1000mAh)", 1000),
        ("LiPo (2000mAh)", 2000),
        ("18650 (3000mAh)", 3000),
    ]
    
    print(f"\n  Estimasi Battery Life:")
    for name, capacity in batteries:
        hours = capacity / grand_total if grand_total > 0 else 0
        print(f"    {name:20s}: {hours:.1f} jam ({hours/24:.1f} hari)")
    
    # Event timeline
    enable_events = [e for e in events if e.get("event") == "enable"]
    disable_events = [e for e in events if e.get("event") == "disable"]
    
    print(f"\n  Event Summary:")
    print(f"    Enable events  : {len(enable_events)}")
    print(f"    Disable events : {len(disable_events)}")
    
    print("\n" + "=" * 60)

if __name__ == "__main__":
    monitor_peripherals()

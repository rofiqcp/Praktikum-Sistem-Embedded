"""
Debug & Analysis Tool - ESP32_12_Power_Budget_Analysis
Power budget calculator, state timeline plot, battery life estimation.

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
LOG_FILE = "power_budget_log.json"

def parse_serial_line(line):
    """Parse satu baris output serial ESP32."""
    data = {"timestamp": datetime.now().isoformat(), "raw": line}
    
    # Parse siklus
    m = re.search(r"Siklus #(\d+)", line)
    if m:
        data["cycle"] = int(m.group(1))
    
    # Parse waktu fase
    m = re.search(r"Sensor Read\s*:\s*(\d+)\s*us\s*\(\s*([\d.]+)\s*ms\)", line)
    if m:
        data["sensor_us"] = int(m.group(1))
        data["sensor_ms"] = float(m.group(2))
    
    m = re.search(r"Processing\s*:\s*(\d+)\s*us\s*\(\s*([\d.]+)\s*ms\)", line)
    if m:
        data["process_us"] = int(m.group(1))
        data["process_ms"] = float(m.group(2))
    
    m = re.search(r"Total Aktif\s*:\s*(\d+)\s*us\s*\(\s*([\d.]+)\s*ms\)", line)
    if m:
        data["active_us"] = int(m.group(1))
        data["active_ms"] = float(m.group(2))
    
    # Parse duty cycle
    m = re.search(r"Duty Cycle\s*:\s*([\d.]+)%", line)
    if m:
        data["duty_cycle"] = float(m.group(1))
    
    # Parse arus
    m = re.search(r"Arus rata-rata\s*:\s*([\d.]+)\s*mA", line)
    if m:
        data["avg_current"] = float(m.group(1))
    
    # Parse battery life
    m = re.search(r"Hari\s*:\s*([\d.]+)", line)
    if m:
        data["battery_days"] = float(m.group(1))
    
    m = re.search(r"Bulan\s*:\s*([\d.]+)", line)
    if m:
        data["battery_months"] = float(m.group(1))
    
    # Parse skenario tabel
    m = re.search(r"([\w\s()]+)\|\s*([\d.]+)\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)", line)
    if m:
        name = m.group(1).strip()
        if name and name != "Skenario" and "-" not in name:
            data["scenario"] = {
                "name": name,
                "current_ma": float(m.group(2)),
                "days": float(m.group(3)),
                "months": float(m.group(4))
            }
    
    return data

def monitor_power_budget():
    """Monitor power budget analysis."""
    print(f"=== Power Budget Analysis Monitor ===")
    print(f"Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f"Log file: {LOG_FILE}")
    print("Tekan Ctrl+C untuk berhenti\n")
    
    cycles = []
    current_cycle = {}
    scenarios = []
    
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
                
                # Warna berdasarkan kategori
                if "LAPORAN" in line or "===" in line:
                    print(f"  \033[96m{line}\033[0m")
                elif "Battery" in line or "battery" in line:
                    print(f"  \033[93m{line}\033[0m")
                elif "Arus" in line:
                    print(f"  \033[92m{line}\033[0m")
                else:
                    print(f"  {line}")
                
                parsed = parse_serial_line(line)
                
                # Kumpulkan data per siklus
                if "cycle" in parsed:
                    if current_cycle:
                        cycles.append(current_cycle)
                    current_cycle = {"cycle": parsed["cycle"], "time": parsed["timestamp"]}
                
                for key in ["sensor_ms", "process_ms", "active_ms", "duty_cycle", "avg_current", "battery_days"]:
                    if key in parsed:
                        current_cycle[key] = parsed[key]
                
                if "scenario" in parsed:
                    scenarios.append(parsed["scenario"])
                    
    except serial.SerialException as e:
        print(f"\nError serial: {e}")
    except KeyboardInterrupt:
        if current_cycle:
            cycles.append(current_cycle)
        print("\n\nMonitoring dihentikan.")
    finally:
        log_data = {
            "session": datetime.now().isoformat(),
            "cycles": cycles,
            "scenarios": scenarios
        }
        with open(LOG_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Data tersimpan ke {LOG_FILE}")
        
        print_analysis_dashboard(cycles, scenarios)

def print_analysis_dashboard(cycles, scenarios):
    """Tampilkan dashboard analisis power budget."""
    print("\n" + "=" * 70)
    print("  POWER BUDGET ANALYSIS DASHBOARD")
    print("=" * 70)
    
    if not cycles:
        print("Tidak ada data siklus untuk dianalisis.")
        return
    
    # Timing analysis
    active_times = [c.get("active_ms", 0) for c in cycles if "active_ms" in c]
    sensor_times = [c.get("sensor_ms", 0) for c in cycles if "sensor_ms" in c]
    process_times = [c.get("process_ms", 0) for c in cycles if "process_ms" in c]
    duty_cycles = [c.get("duty_cycle", 0) for c in cycles if "duty_cycle" in c]
    avg_currents = [c.get("avg_current", 0) for c in cycles if "avg_current" in c]
    
    print(f"\n  Timing Analysis ({len(cycles)} siklus):")
    
    if active_times:
        print(f"    Waktu Aktif:")
        print(f"      Min    : {min(active_times):.2f} ms")
        print(f"      Max    : {max(active_times):.2f} ms")
        print(f"      Avg    : {sum(active_times)/len(active_times):.2f} ms")
    
    if sensor_times:
        print(f"    Waktu Sensor:")
        print(f"      Min    : {min(sensor_times):.2f} ms")
        print(f"      Max    : {max(sensor_times):.2f} ms")
        print(f"      Avg    : {sum(sensor_times)/len(sensor_times):.2f} ms")
    
    # Active time trend
    if active_times:
        print(f"\n  Trend Waktu Aktif per Siklus:")
        max_t = max(active_times) if active_times else 1
        for i, t in enumerate(active_times[-20:]):
            bar_len = int(t / max_t * 30) if max_t > 0 else 0
            bar = "█" * bar_len
            print(f"    #{i+1:3d}: {bar:30s} {t:.2f} ms")
    
    # Duty cycle trend
    if duty_cycles:
        print(f"\n  Duty Cycle Analysis:")
        print(f"    Avg    : {sum(duty_cycles)/len(duty_cycles):.6f}%")
        print(f"    Min    : {min(duty_cycles):.6f}%")
        print(f"    Max    : {max(duty_cycles):.6f}%")
    
    # Current analysis
    if avg_currents:
        avg_cur = sum(avg_currents) / len(avg_currents)
        print(f"\n  Current Analysis:")
        print(f"    Arus rata-rata: {avg_cur:.4f} mA")
        
        # Battery life untuk berbagai kapasitas
        print(f"\n  Battery Life Estimation (avg current: {avg_cur:.4f} mA):")
        batteries = [
            ("CR2032 (220mAh)", 220),
            ("AAA NiMH (800mAh)", 800),
            ("AA NiMH (2000mAh)", 2000),
            ("LiPo 3.7V (1000mAh)", 1000),
            ("LiPo 3.7V (2000mAh)", 2000),
            ("LiPo 3.7V (5000mAh)", 5000),
            ("18650 (3000mAh)", 3000),
        ]
        
        for name, cap in batteries:
            hours = cap / avg_cur if avg_cur > 0 else 0
            days = hours / 24
            months = days / 30
            years = days / 365
            
            if years >= 1:
                time_str = f"{years:.1f} tahun"
            elif months >= 1:
                time_str = f"{months:.1f} bulan"
            else:
                time_str = f"{days:.1f} hari"
            
            bar_len = min(int(days / 30), 30)
            bar = "▓" * bar_len
            print(f"    {name:25s}: {bar:30s} {time_str}")
    
    # Scenario comparison
    if scenarios:
        unique_scenarios = {}
        for s in scenarios:
            unique_scenarios[s["name"]] = s
        
        print(f"\n  Perbandingan Skenario:")
        print(f"    {'Skenario':<25s} | {'Arus(mA)':>9s} | {'Hari':>7s} | {'Bulan':>6s}")
        print(f"    {'-'*25}-+-{'-'*9}-+-{'-'*7}-+-{'-'*6}")
        
        for name, s in unique_scenarios.items():
            print(f"    {name:<25s} | {s['current_ma']:>9.4f} | {s['days']:>7.1f} | {s['months']:>6.1f}")
    
    # Power state pie chart ASCII
    if active_times and duty_cycles:
        avg_duty = sum(duty_cycles) / len(duty_cycles)
        sleep_pct = 100 - avg_duty
        
        print(f"\n  Distribusi Waktu (Pie Chart ASCII):")
        active_bar = max(1, int(avg_duty / 100 * 50))
        sleep_bar = 50 - active_bar
        
        print(f"    Aktif : {'█' * active_bar} {avg_duty:.4f}%")
        print(f"    Sleep : {'░' * sleep_bar} {sleep_pct:.4f}%")
        
        # Power distribution
        if avg_currents:
            avg_cur = sum(avg_currents) / len(avg_currents)
            active_power = 50.0 * (avg_duty / 100)
            sleep_power = 0.01 * (sleep_pct / 100)
            total_power = active_power + sleep_power
            
            print(f"\n  Distribusi Daya:")
            if total_power > 0:
                print(f"    Aktif : {'█' * int(active_power/total_power*50)} {active_power:.4f} mA ({active_power/total_power*100:.1f}%)")
                print(f"    Sleep : {'░' * max(1,int(sleep_power/total_power*50))} {sleep_power:.6f} mA ({sleep_power/total_power*100:.4f}%)")
    
    print("\n  Rekomendasi:")
    if avg_currents:
        avg_cur = sum(avg_currents) / len(avg_currents)
        if avg_cur < 0.1:
            print("    ✓ Sangat efisien! Cocok untuk aplikasi IoT bertenaga baterai.")
        elif avg_cur < 1.0:
            print("    ✓ Efisien. Pertimbangkan optimasi sensor read jika perlu lebih hemat.")
        else:
            print("    ! Konsumsi cukup tinggi. Pertimbangkan perpanjang interval sleep.")
    
    print("\n" + "=" * 70)

if __name__ == "__main__":
    monitor_power_budget()

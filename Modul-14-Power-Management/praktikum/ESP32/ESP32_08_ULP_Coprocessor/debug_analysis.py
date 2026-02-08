"""
Debug & Analysis Tool - ESP32_08_ULP_Coprocessor
ULP monitoring dashboard dan ADC threshold analysis.

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
LOG_FILE = "ulp_coprocessor_log.json"

def parse_serial_line(line):
    """Parse satu baris output serial ESP32."""
    data = {"timestamp": datetime.now().isoformat(), "raw": line}
    
    # Parse boot count
    m = re.search(r"Boot ke-(\d+)", line)
    if m:
        data["boot_count"] = int(m.group(1))
    
    # Parse ULP ADC check
    m = re.search(r"ULP checked ADC: value=(\d+), threshold=(\d+).*cek ke-(\d+)", line)
    if m:
        data["adc_value"] = int(m.group(1))
        data["threshold"] = int(m.group(2))
        data["check_number"] = int(m.group(3))
    
    # Parse threshold exceeded
    if "THRESHOLD EXCEEDED" in line:
        data["threshold_exceeded"] = True
        m = re.search(r"exceed ke-(\d+)", line)
        if m:
            data["exceed_count"] = int(m.group(1))
    
    # Parse statistik
    m = re.search(r"Total pengecekan ULP\s*:\s*(\d+)", line)
    if m:
        data["total_checks"] = int(m.group(1))
    
    m = re.search(r"Threshold exceeded\s*:\s*(\d+)", line)
    if m:
        data["total_exceeds"] = int(m.group(1))
    
    m = re.search(r"Rata-rata ADC:\s*(\d+)", line)
    if m:
        data["avg_adc"] = int(m.group(1))
    
    return data

def monitor_ulp():
    """Monitor ULP simulator dan tampilkan dashboard."""
    print(f"=== ULP Coprocessor Monitor ===")
    print(f"Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f"Log file: {LOG_FILE}")
    print("Tekan Ctrl+C untuk berhenti\n")
    
    adc_readings = []
    threshold_events = []
    
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
                
                # Warna output berdasarkan tipe
                if "THRESHOLD EXCEEDED" in line:
                    print(f"  \033[91m[!] {line}\033[0m")
                elif "CPU UTAMA" in line:
                    print(f"  \033[93m[*] {line}\033[0m")
                elif "ULP checked" in line:
                    print(f"  \033[92m[U] {line}\033[0m")
                else:
                    print(f"  [ ] {line}")
                
                parsed = parse_serial_line(line)
                
                if "adc_value" in parsed:
                    adc_readings.append({
                        "time": parsed["timestamp"],
                        "value": parsed["adc_value"],
                        "threshold": parsed["threshold"],
                        "check_num": parsed.get("check_number", 0)
                    })
                
                if parsed.get("threshold_exceeded"):
                    threshold_events.append({
                        "time": parsed["timestamp"],
                        "count": parsed.get("exceed_count", 0)
                    })
                    
    except serial.SerialException as e:
        print(f"\nError serial: {e}")
    except KeyboardInterrupt:
        print("\n\nMonitoring dihentikan.")
    finally:
        # Simpan log
        log_data = {
            "session": datetime.now().isoformat(),
            "adc_readings": adc_readings,
            "threshold_events": threshold_events,
            "total_readings": len(adc_readings),
            "total_threshold_events": len(threshold_events)
        }
        with open(LOG_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Data tersimpan ke {LOG_FILE}")
        
        # Print analisis
        print_threshold_analysis(adc_readings, threshold_events)

def print_threshold_analysis(readings, events):
    """Analisis threshold ADC dan tampilkan dashboard."""
    print("\n" + "=" * 60)
    print("  ULP MONITORING DASHBOARD")
    print("=" * 60)
    
    if not readings:
        print("Tidak ada data untuk dianalisis.")
        return
    
    values = [r["value"] for r in readings]
    threshold = readings[0]["threshold"]
    above = [v for v in values if v > threshold]
    below = [v for v in values if v <= threshold]
    
    print(f"\nTotal pembacaan  : {len(values)}")
    print(f"Threshold        : {threshold}")
    print(f"Di atas threshold: {len(above)} ({len(above)/len(values)*100:.1f}%)")
    print(f"Di bawah/sama    : {len(below)} ({len(below)/len(values)*100:.1f}%)")
    
    print(f"\nStatistik ADC:")
    print(f"  Min   : {min(values)}")
    print(f"  Max   : {max(values)}")
    print(f"  Rata2 : {sum(values)/len(values):.1f}")
    
    # Timeline ASCII
    print(f"\nTimeline ADC (terbaru {min(30, len(values))} sampel):")
    recent = values[-30:]
    max_val = max(max(recent), threshold)
    width = 40
    
    for i, val in enumerate(recent):
        bar_len = int(val / max_val * width) if max_val > 0 else 0
        thresh_pos = int(threshold / max_val * width) if max_val > 0 else 0
        
        bar = list("." * width)
        for j in range(bar_len):
            bar[j] = "#" if val <= threshold else "!"
        if thresh_pos < width:
            bar[thresh_pos] = "|"
        
        marker = " <<<" if val > threshold else ""
        print(f"  {i+1:3d}: {''.join(bar)} {val:4d}{marker}")
    
    print(f"\n  Legenda: # = normal, ! = melebihi threshold, | = threshold")
    
    # Threshold events
    if events:
        print(f"\nEvent Threshold Exceeded ({len(events)} kali):")
        for evt in events[-10:]:
            print(f"  [{evt['time'][-8:]}] Exceed ke-{evt['count']}")
    
    print("\n" + "=" * 60)

if __name__ == "__main__":
    monitor_ulp()

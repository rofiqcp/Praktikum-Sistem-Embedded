"""
Debug & Analysis Tool - ESP32_07_Touch_Wakeup
Touch event logger dan sensitivity analysis plot.

Penggunaan: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import re
import time
import json
from datetime import datetime
from collections import defaultdict

# Konfigurasi
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD_RATE = 115200
LOG_FILE = "touch_wakeup_log.json"

def parse_serial_line(line):
    """Parse satu baris output serial ESP32."""
    data = {"timestamp": datetime.now().isoformat(), "raw": line}
    
    # Parse boot count
    m = re.search(r"Boot ke-(\d+)", line)
    if m:
        data["boot_count"] = int(m.group(1))
    
    # Parse touch values
    m = re.search(r"raw=(\d+),\s*filtered=(\d+),\s*threshold=(\d+)", line)
    if m:
        data["touch_raw"] = int(m.group(1))
        data["touch_filtered"] = int(m.group(2))
        data["threshold"] = int(m.group(3))
    
    # Parse wakeup cause
    if "TOUCH PAD" in line:
        data["wakeup_cause"] = "touch"
        m = re.search(r"touch wakeup ke-(\d+)", line)
        if m:
            data["touch_wakeup_count"] = int(m.group(1))
    elif "TIMER" in line:
        data["wakeup_cause"] = "timer"
    elif "Boot pertama" in line:
        data["wakeup_cause"] = "reset"
    
    # Parse touch status
    if "PAD TERSENTUH" in line:
        data["touch_detected"] = True
    elif "tidak tersentuh" in line:
        data["touch_detected"] = False
    
    return data

def monitor_touch_events():
    """Monitor serial dan catat event sentuhan."""
    print(f"=== Touch Wakeup Logger ===")
    print(f"Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f"Log file: {LOG_FILE}")
    print("Tekan Ctrl+C untuk berhenti\n")
    
    all_data = []
    touch_values = []
    boot_events = []
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Tunggu koneksi stabil
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode("utf-8", errors="replace").strip()
                except Exception:
                    continue
                
                if not line:
                    continue
                
                print(f"[{datetime.now().strftime('%H:%M:%S')}] {line}")
                
                parsed = parse_serial_line(line)
                all_data.append(parsed)
                
                # Kumpulkan data touch
                if "touch_raw" in parsed:
                    touch_values.append({
                        "time": parsed["timestamp"],
                        "raw": parsed["touch_raw"],
                        "filtered": parsed["touch_filtered"],
                        "threshold": parsed["threshold"],
                        "touched": parsed.get("touch_detected", False)
                    })
                
                # Catat event boot
                if "wakeup_cause" in parsed:
                    boot_events.append({
                        "time": parsed["timestamp"],
                        "cause": parsed["wakeup_cause"],
                        "boot_count": parsed.get("boot_count", 0)
                    })
                    
    except serial.SerialException as e:
        print(f"\nError serial: {e}")
    except KeyboardInterrupt:
        print("\n\nMonitoring dihentikan.")
    finally:
        # Simpan data
        log_data = {
            "session": datetime.now().isoformat(),
            "touch_values": touch_values,
            "boot_events": boot_events,
            "total_samples": len(touch_values)
        }
        with open(LOG_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Data tersimpan ke {LOG_FILE}")
        
        # Print analisis
        print_sensitivity_analysis(touch_values, boot_events)

def print_sensitivity_analysis(touch_values, boot_events):
    """Analisis sensitivitas touch pad dan tampilkan ringkasan."""
    print("\n" + "=" * 60)
    print("  ANALISIS SENSITIVITAS TOUCH PAD")
    print("=" * 60)
    
    if not touch_values:
        print("Tidak ada data touch untuk dianalisis.")
        return
    
    raw_vals = [v["raw"] for v in touch_values]
    filtered_vals = [v["filtered"] for v in touch_values]
    threshold = touch_values[0]["threshold"]
    touched_count = sum(1 for v in touch_values if v.get("touched", False))
    
    print(f"\nTotal sampel       : {len(touch_values)}")
    print(f"Threshold          : {threshold}")
    print(f"Sentuhan terdeteksi: {touched_count}/{len(touch_values)}")
    
    print(f"\nNilai RAW:")
    print(f"  Min    : {min(raw_vals)}")
    print(f"  Max    : {max(raw_vals)}")
    print(f"  Rata2  : {sum(raw_vals)/len(raw_vals):.1f}")
    
    print(f"\nNilai FILTERED:")
    print(f"  Min    : {min(filtered_vals)}")
    print(f"  Max    : {max(filtered_vals)}")
    print(f"  Rata2  : {sum(filtered_vals)/len(filtered_vals):.1f}")
    
    # Histogram sederhana ASCII
    print(f"\nDistribusi nilai filtered (ASCII):")
    bucket_size = 50
    buckets = defaultdict(int)
    for v in filtered_vals:
        bucket = (v // bucket_size) * bucket_size
        buckets[bucket] += 1
    
    for bucket in sorted(buckets.keys()):
        bar = "#" * buckets[bucket]
        label = f"{bucket:4d}-{bucket+bucket_size-1:4d}"
        marker = " <-- threshold" if bucket <= threshold < bucket + bucket_size else ""
        print(f"  {label}: {bar} ({buckets[bucket]}){marker}")
    
    # Boot events
    if boot_events:
        print(f"\nEvent Boot ({len(boot_events)} total):")
        cause_count = defaultdict(int)
        for evt in boot_events:
            cause_count[evt["cause"]] += 1
        for cause, count in cause_count.items():
            print(f"  {cause:10s}: {count}x")
    
    # Rekomendasi threshold
    if touched_count > 0 and touched_count < len(touch_values):
        touched_vals = [v["filtered"] for v in touch_values if v.get("touched")]
        untouched_vals = [v["filtered"] for v in touch_values if not v.get("touched")]
        if touched_vals and untouched_vals:
            optimal = (max(touched_vals) + min(untouched_vals)) // 2
            print(f"\nRekomendasi threshold optimal: {optimal}")
            print(f"  (antara max_touched={max(touched_vals)} dan min_untouched={min(untouched_vals)})")

    print("\n" + "=" * 60)

if __name__ == "__main__":
    monitor_touch_events()

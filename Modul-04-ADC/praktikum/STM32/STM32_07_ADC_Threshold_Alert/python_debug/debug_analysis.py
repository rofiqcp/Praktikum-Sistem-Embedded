"""
=============================================================================
 Program     : STM32_07_ADC_Threshold_Alert - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk ADC threshold alert dengan analog
               watchdog, menampilkan status dan grafik threshold.
=============================================================================
"""

import serial
import csv
import time
import re
import os
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
from collections import deque

# ======================== KONFIGURASI ========================
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILENAME = 'adc_threshold_alert_log.csv'
MAX_DATA_POINTS = 200
THRESHOLD_HIGH = 3000
THRESHOLD_LOW = 1000

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
adc_values = deque(maxlen=MAX_DATA_POINTS)
alert_counts = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [n] ADC:val MV:val STATUS:str ALERT_CNT:val"""
    match = re.search(
        r'\[(\d+)\]\s*ADC:(\d+)\s*MV:(\d+)\s*STATUS:(\w+)\s*ALERT_CNT:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'adc': int(match.group(2)),
            'mv': int(match.group(3)),
            'status': match.group(4),
            'alert_cnt': int(match.group(5))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'adc', 'mv', 'status', 'alert_cnt']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update grafik threshold"""
    ax.clear()
    if len(timestamps) > 0:
        t = list(timestamps)
        vals = list(adc_values)

        # Warnai titik berdasarkan status
        colors = ['red' if v > THRESHOLD_HIGH or v < THRESHOLD_LOW else 'green' for v in vals]
        ax.scatter(t, vals, c=colors, s=15, zorder=5)
        ax.plot(t, vals, 'b-', alpha=0.3, linewidth=1)

        # Garis threshold
        ax.axhline(y=THRESHOLD_HIGH, color='r', linestyle='--', linewidth=2, label=f'Batas Atas ({THRESHOLD_HIGH})')
        ax.axhline(y=THRESHOLD_LOW, color='orange', linestyle='--', linewidth=2, label=f'Batas Bawah ({THRESHOLD_LOW})')

        # Area aman
        ax.axhspan(THRESHOLD_LOW, THRESHOLD_HIGH, alpha=0.1, color='green', label='Range Normal')

    ax.set_title('ADC Threshold Alert - Analog Watchdog')
    ax.set_xlabel('Waktu (detik)')
    ax.set_ylabel('Nilai ADC')
    ax.set_ylim(-100, 4200)
    ax.legend(loc='upper right')
    ax.grid(True, alpha=0.3)

def main():
    global ax
    print("=" * 60)
    print("  STM32 ADC Threshold Alert - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, ax = plt.subplots(figsize=(12, 6))
    plt.tight_layout()

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=500, cache_frame_data=False)

        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  >> {line}")
                        data = parse_line(line)
                        if data:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            adc_values.append(data['adc'])
                            alert_counts.append(data['alert_cnt'])
                            save_to_csv(data, CSV_FILENAME)
                except Exception as e:
                    print(f"[ERROR] {e}")
            plt.pause(0.01)

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print(f"\n[INFO] Program dihentikan. Data: {CSV_FILENAME}")
    finally:
        try:
            ser.close()
        except:
            pass

if __name__ == '__main__':
    main()

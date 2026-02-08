"""
=============================================================================
 Program     : STM32_08_ADC_Battery_Monitor - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk battery monitor, menampilkan
               tegangan baterai, persentase, dan status secara real-time.
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
CSV_FILENAME = 'adc_battery_monitor_log.csv'
MAX_DATA_POINTS = 200

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
battery_mv_values = deque(maxlen=MAX_DATA_POINTS)
battery_pct_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [n] ADC:val BATT_MV:val BATT_PCT:val STATUS:str"""
    match = re.search(
        r'\[(\d+)\]\s*ADC:(\d+)\s*BATT_MV:(\d+)\s*BATT_PCT:(\d+)\s*STATUS:(\w+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'adc': int(match.group(2)),
            'batt_mv': int(match.group(3)),
            'batt_pct': int(match.group(4)),
            'status': match.group(5)
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'adc', 'batt_mv', 'batt_pct', 'status']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update grafik baterai"""
    ax1.clear()
    ax2.clear()
    if len(timestamps) > 0:
        t = list(timestamps)

        # Tegangan baterai
        ax1.plot(t, list(battery_mv_values), 'b-o', markersize=3, linewidth=2, label='Tegangan (mV)')
        ax1.axhline(y=4200, color='g', linestyle='--', alpha=0.7, label='Penuh (4.2V)')
        ax1.axhline(y=3700, color='y', linestyle='--', alpha=0.7, label='Nominal (3.7V)')
        ax1.axhline(y=3000, color='r', linestyle='--', alpha=0.7, label='Kritis (3.0V)')
        ax1.set_title('Tegangan Baterai')
        ax1.set_ylabel('Tegangan (mV)')
        ax1.set_ylim(2500, 4500)
        ax1.legend(loc='upper right', fontsize=8)
        ax1.grid(True, alpha=0.3)

        # Persentase baterai
        pct_list = list(battery_pct_values)
        colors = ['green' if p > 50 else 'orange' if p > 20 else 'red' for p in pct_list]
        ax2.bar(t, pct_list, width=0.5, color=colors, alpha=0.7)
        ax2.set_title('Persentase Baterai')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Persentase (%)')
        ax2.set_ylim(0, 110)
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 Battery Monitor - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.tight_layout(pad=3.0)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=2000, cache_frame_data=False)

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
                            battery_mv_values.append(data['batt_mv'])
                            battery_pct_values.append(data['batt_pct'])
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

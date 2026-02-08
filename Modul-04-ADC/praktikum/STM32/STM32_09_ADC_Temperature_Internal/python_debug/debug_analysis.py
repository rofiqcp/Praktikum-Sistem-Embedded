"""
=============================================================================
 Program     : STM32_09_ADC_Temperature_Internal - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk sensor suhu internal STM32,
               menampilkan suhu dan tegangan sensor secara real-time.
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
CSV_FILENAME = 'adc_temperature_internal_log.csv'
MAX_DATA_POINTS = 200

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
temp_values = deque(maxlen=MAX_DATA_POINTS)
vsense_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [n] ADC:val VSENSE_MV:val TEMP_X10:val"""
    match = re.search(r'\[(\d+)\]\s*ADC:(\d+)\s*VSENSE_MV:(\d+)\s*TEMP_X10:(-?\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'adc': int(match.group(2)),
            'vsense_mv': int(match.group(3)),
            'temp_x10': int(match.group(4))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'adc', 'vsense_mv', 'temp_c']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            'counter': data['counter'],
            'adc': data['adc'],
            'vsense_mv': data['vsense_mv'],
            'temp_c': data['temp_x10'] / 10.0
        })

def update_plot(frame):
    """Update grafik suhu"""
    ax1.clear()
    ax2.clear()
    if len(timestamps) > 0:
        t = list(timestamps)

        # Grafik suhu
        temps = [v / 10.0 for v in temp_values]
        ax1.plot(t, temps, 'r-o', markersize=3, linewidth=2, label='Suhu (°C)')
        ax1.set_title('Suhu Internal STM32')
        ax1.set_ylabel('Suhu (°C)')
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # Grafik tegangan sensor
        ax2.plot(t, list(vsense_values), 'b-o', markersize=3, linewidth=2, label='Vsense (mV)')
        ax2.axhline(y=1430, color='g', linestyle='--', alpha=0.7, label='V25 (1430mV)')
        ax2.set_title('Tegangan Sensor Suhu')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Tegangan (mV)')
        ax2.legend()
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 Internal Temperature - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.tight_layout(pad=3.0)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=1000, cache_frame_data=False)

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
                            temp_values.append(data['temp_x10'])
                            vsense_values.append(data['vsense_mv'])
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

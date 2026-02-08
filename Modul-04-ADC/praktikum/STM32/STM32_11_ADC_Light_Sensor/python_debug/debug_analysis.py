"""
=============================================================================
 Program     : STM32_11_ADC_Light_Sensor - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk sensor cahaya LDR, menampilkan
               lux, persentase, dan level cahaya secara real-time.
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
CSV_FILENAME = 'adc_light_sensor_log.csv'
MAX_DATA_POINTS = 200

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
lux_values = deque(maxlen=MAX_DATA_POINTS)
pct_values = deque(maxlen=MAX_DATA_POINTS)
adc_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [n] ADC:val LUX:val PCT:val LEVEL:str"""
    match = re.search(r'\[(\d+)\]\s*ADC:(\d+)\s*LUX:(\d+)\s*PCT:(\d+)\s*LEVEL:(\S+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'adc': int(match.group(2)),
            'lux': int(match.group(3)),
            'pct': int(match.group(4)),
            'level': match.group(5)
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'adc', 'lux', 'pct', 'level']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update grafik sensor cahaya"""
    ax1.clear()
    ax2.clear()
    if len(timestamps) > 0:
        t = list(timestamps)

        # Grafik Lux
        ax1.plot(t, list(lux_values), 'y-o', markersize=3, linewidth=2, label='Lux')
        ax1.set_title('Estimasi Intensitas Cahaya (Lux)')
        ax1.set_ylabel('Lux')
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # Grafik Persentase Cahaya
        pct_list = list(pct_values)
        colors_map = {'GELAP': 'black', 'REDUP': 'gray', 'NORMAL': 'yellow',
                       'TERANG': 'orange', 'SANGAT_TERANG': 'red'}
        ax2.fill_between(t, 0, pct_list, alpha=0.3, color='yellow')
        ax2.plot(t, pct_list, 'orange', linewidth=2, label='Cahaya (%)')

        # Garis klasifikasi
        ax2.axhline(y=12, color='gray', linestyle=':', alpha=0.5, label='Gelap')
        ax2.axhline(y=37, color='blue', linestyle=':', alpha=0.5, label='Redup')
        ax2.axhline(y=61, color='green', linestyle=':', alpha=0.5, label='Normal')
        ax2.axhline(y=85, color='red', linestyle=':', alpha=0.5, label='Terang')

        ax2.set_title('Persentase Cahaya')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Cahaya (%)')
        ax2.set_ylim(0, 105)
        ax2.legend(loc='upper right', fontsize=8)
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 Light Sensor (LDR) - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.tight_layout(pad=3.0)

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
                            lux_values.append(data['lux'])
                            pct_values.append(data['pct'])
                            adc_values.append(data['adc'])
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

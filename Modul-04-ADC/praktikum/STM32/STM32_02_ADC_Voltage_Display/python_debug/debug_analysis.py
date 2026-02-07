"""
=============================================================================
 Program     : STM32_02_ADC_Voltage_Display - Debug & Analisis
 Modul       : 04 - ADC (Analog to Digital Converter)
 Deskripsi   : Serial monitor untuk membaca data ADC dan tegangan,
               menyimpan ke CSV, dan menampilkan grafik real-time.
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
CSV_FILENAME = 'adc_voltage_display_log.csv'
MAX_DATA_POINTS = 200

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
adc_values = deque(maxlen=MAX_DATA_POINTS)
voltage_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing data serial: [counter] ADC_RAW:value VOLTAGE_MV:value"""
    match = re.search(r'\[(\d+)\]\s*ADC_RAW:(\d+)\s*VOLTAGE_MV:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'adc_raw': int(match.group(2)),
            'voltage_mv': int(match.group(3))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke file CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'adc_raw', 'voltage_mv']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            'counter': data['counter'],
            'adc_raw': data['adc_raw'],
            'voltage_mv': data['voltage_mv']
        })

def update_plot(frame):
    """Update grafik secara real-time"""
    ax1.clear()
    ax2.clear()

    if len(timestamps) > 0:
        ax1.plot(list(timestamps), list(adc_values), 'b-o', markersize=2, label='ADC Raw')
        ax1.set_ylim(-100, 4200)
        ax1.set_title('Nilai ADC Raw (0-4095)')
        ax1.set_ylabel('Nilai ADC')
        ax1.legend(loc='upper right')
        ax1.grid(True, alpha=0.3)

        ax2.plot(list(timestamps), list(voltage_values), 'r-o', markersize=2, label='Tegangan (mV)')
        ax2.set_ylim(-100, 3500)
        ax2.set_title('Tegangan (milivolt)')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Tegangan (mV)')
        ax2.legend(loc='upper right')
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 ADC Voltage Display - Serial Monitor & Analyzer")
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
                            adc_values.append(data['adc_raw'])
                            voltage_values.append(data['voltage_mv'])
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

"""
=============================================================================
 Program     : STM32_06_ADC_Continuous_DMA - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk ADC dengan DMA circular buffer,
               menampilkan statistik (avg/min/max) dan grafik real-time.
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
CSV_FILENAME = 'adc_continuous_dma_log.csv'
MAX_DATA_POINTS = 200

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
avg_values = deque(maxlen=MAX_DATA_POINTS)
min_values = deque(maxlen=MAX_DATA_POINTS)
max_values = deque(maxlen=MAX_DATA_POINTS)
voltage_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [n] DMA_AVG:val DMA_MIN:val DMA_MAX:val VOLTAGE_MV:val"""
    match = re.search(
        r'\[(\d+)\]\s*DMA_AVG:(\d+)\s*DMA_MIN:(\d+)\s*DMA_MAX:(\d+)\s*VOLTAGE_MV:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'dma_avg': int(match.group(2)),
            'dma_min': int(match.group(3)),
            'dma_max': int(match.group(4)),
            'voltage_mv': int(match.group(5))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'dma_avg', 'dma_min', 'dma_max', 'voltage_mv']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update grafik DMA"""
    ax1.clear()
    ax2.clear()
    if len(timestamps) > 0:
        t = list(timestamps)
        ax1.fill_between(t, list(min_values), list(max_values), alpha=0.3, color='blue', label='Min-Max Range')
        ax1.plot(t, list(avg_values), 'r-', linewidth=2, label='DMA Average')
        ax1.set_title('ADC DMA - Rata-rata & Range (Min-Max)')
        ax1.set_ylabel('Nilai ADC')
        ax1.set_ylim(-100, 4200)
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        ax2.plot(t, list(voltage_values), 'g-', linewidth=2, label='Tegangan (mV)')
        ax2.set_title('Tegangan dari DMA Average')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Tegangan (mV)')
        ax2.set_ylim(-100, 3500)
        ax2.legend()
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 ADC Continuous DMA - Serial Monitor & Analyzer")
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
                            avg_values.append(data['dma_avg'])
                            min_values.append(data['dma_min'])
                            max_values.append(data['dma_max'])
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

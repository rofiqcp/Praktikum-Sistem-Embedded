"""
=============================================================================
 Program     : STM32_12_ADC_Statistical_Analysis - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk analisis statistik ADC (min/max/mean/
               stddev/histogram), menyimpan ke CSV, dan menampilkan grafik.
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
CSV_FILENAME = 'adc_statistical_analysis_log.csv'
MAX_DATA_POINTS = 50

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
min_values = deque(maxlen=MAX_DATA_POINTS)
max_values = deque(maxlen=MAX_DATA_POINTS)
mean_values = deque(maxlen=MAX_DATA_POINTS)
stddev_values = deque(maxlen=MAX_DATA_POINTS)
latest_histogram = []
start_time = time.time()

def parse_stats_line(line):
    """Parsing: [n] MIN:val MAX:val MEAN:val STDDEV_X100:val RANGE:val"""
    match = re.search(
        r'\[(\d+)\]\s*MIN:(\d+)\s*MAX:(\d+)\s*MEAN:(\d+)\s*STDDEV_X100:(\d+)\s*RANGE:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'min': int(match.group(2)),
            'max': int(match.group(3)),
            'mean': int(match.group(4)),
            'stddev_x100': int(match.group(5)),
            'range': int(match.group(6))
        }
    return None

def parse_histogram_line(line):
    """Parsing: HIST:val1,val2,..."""
    match = re.search(r'HIST:([\d,]+)', line)
    if match:
        return [int(x) for x in match.group(1).split(',')]
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'min', 'max', 'mean', 'stddev', 'range']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            'counter': data['counter'],
            'min': data['min'],
            'max': data['max'],
            'mean': data['mean'],
            'stddev': data['stddev_x100'] / 100.0,
            'range': data['range']
        })

def update_plot(frame):
    """Update grafik statistik"""
    ax1.clear()
    ax2.clear()

    if len(timestamps) > 0:
        t = list(timestamps)

        # Grafik statistik (mean dengan range min-max)
        ax1.fill_between(t, list(min_values), list(max_values),
                         alpha=0.2, color='blue', label='Range (Min-Max)')
        ax1.plot(t, list(mean_values), 'r-o', markersize=4, linewidth=2, label='Mean')

        # Error bars untuk stddev
        stddevs = [s / 100.0 for s in stddev_values]
        means = list(mean_values)
        upper = [m + s for m, s in zip(means, stddevs)]
        lower = [m - s for m, s in zip(means, stddevs)]
        ax1.fill_between(t, lower, upper, alpha=0.3, color='red', label='±1 StdDev')

        ax1.set_title('Analisis Statistik ADC')
        ax1.set_ylabel('Nilai ADC')
        ax1.legend(loc='upper right')
        ax1.grid(True, alpha=0.3)

    # Histogram terbaru
    if latest_histogram:
        bins_x = list(range(len(latest_histogram)))
        colors = plt.cm.viridis([i / len(latest_histogram) for i in range(len(latest_histogram))])
        ax2.bar(bins_x, latest_histogram, color=colors, edgecolor='black')
        ax2.set_title('Histogram Distribusi Sampel Terakhir')
        ax2.set_xlabel('Bin')
        ax2.set_ylabel('Frekuensi')
        ax2.grid(True, alpha=0.3, axis='y')

def main():
    global ax1, ax2, latest_histogram
    print("=" * 60)
    print("  STM32 ADC Statistical Analysis - Serial Monitor")
    print("=" * 60)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.tight_layout(pad=3.0)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=3000, cache_frame_data=False)

        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  >> {line}")

                        # Coba parse baris statistik
                        stats = parse_stats_line(line)
                        if stats:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            min_values.append(stats['min'])
                            max_values.append(stats['max'])
                            mean_values.append(stats['mean'])
                            stddev_values.append(stats['stddev_x100'])
                            save_to_csv(stats, CSV_FILENAME)

                        # Coba parse baris histogram
                        hist = parse_histogram_line(line)
                        if hist:
                            latest_histogram = hist

                except Exception as e:
                    print(f"[ERROR] {e}")
            plt.pause(0.01)

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print(f"\n[INFO] Program dihentikan. Data: {CSV_FILENAME}")
        if len(mean_values) > 0:
            print(f"\n[RINGKASAN]")
            print(f"  Total analisis  : {len(mean_values)}")
            print(f"  Mean terakhir   : {mean_values[-1]}")
            print(f"  StdDev terakhir : {stddev_values[-1] / 100.0:.2f}")
    finally:
        try:
            ser.close()
        except:
            pass

if __name__ == '__main__':
    main()

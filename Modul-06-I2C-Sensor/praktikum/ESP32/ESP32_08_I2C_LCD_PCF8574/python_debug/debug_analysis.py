"""
ESP32_08_I2C_LCD_PCF8574 - Debug Analysis
Modul 06 - I2C & Sensor

Deskripsi: Parser serial + analisis data LCD I2C PCF8574.
           Memantau counter LCD dan status backlight.
           Menyimpan log ke CSV dan membuat grafik timeline.

Penggunaan:
    python debug_analysis.py [PORT]
    Contoh: python debug_analysis.py /dev/ttyUSB0
"""

import serial
import csv
import time
import sys
import os
from datetime import datetime
from collections import defaultdict

# Konfigurasi default
DEFAULT_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILENAME = 'lcd_pcf8574_data.csv'
PLOT_FILENAME = 'lcd_pcf8574_plot.png'
MAX_SAMPLES = 500

def parse_serial_data(port, duration=60):
    """Membaca dan mem-parsing data serial dari ESP32."""
    data_list = []
    event_log = []

    print(f"[INFO] Membuka port serial {port} pada {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(2)
        ser.reset_input_buffer()
        print(f"[INFO] Port serial terbuka. Membaca data selama {duration} detik...")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti lebih awal.\n")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        return data_list, event_log

    start_time = time.time()
    sample_count = 0

    try:
        while (time.time() - start_time) < duration and sample_count < MAX_SAMPLES:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue

                print(f"  RAW: {line}")

                # Parse format: DATA,counter,baris1,baris2
                if line.startswith('DATA,'):
                    parts = line.split(',')
                    if len(parts) >= 4:
                        try:
                            counter = int(parts[1])
                            baris1 = parts[2]
                            baris2 = parts[3]
                            timestamp = time.time() - start_time

                            entry = {
                                'timestamp': timestamp,
                                'counter': counter,
                                'baris1': baris1,
                                'baris2': baris2
                            }
                            data_list.append(entry)
                            sample_count += 1

                            print(f"  [LCD] Counter: {counter} | "
                                  f"L1: '{baris1}' | L2: '{baris2}'")
                        except (ValueError, IndexError) as e:
                            print(f"  [WARN] Gagal parse: {e}")

                # Deteksi event backlight
                if 'Backlight toggle' in line or 'backlight' in line.lower():
                    event_log.append({
                        'timestamp': time.time() - start_time,
                        'event': 'BACKLIGHT_TOGGLE',
                        'detail': line
                    })
                    print(f"  [EVENT] Backlight toggle terdeteksi")

    except KeyboardInterrupt:
        print("\n[INFO] Pembacaan dihentikan oleh pengguna.")
    finally:
        ser.close()
        print(f"[INFO] Port serial ditutup. Total sampel: {sample_count}")

    return data_list, event_log


def save_to_csv(data_list, event_log, filename):
    """Menyimpan data ke file CSV."""
    filepath = os.path.join(os.path.dirname(__file__), filename)

    # Simpan data utama
    with open(filepath, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile,
                                fieldnames=['timestamp', 'counter', 'baris1', 'baris2'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {filepath} ({len(data_list)} baris)")

    # Simpan event log
    event_file = filepath.replace('.csv', '_events.csv')
    if event_log:
        with open(event_file, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile,
                                    fieldnames=['timestamp', 'event', 'detail'])
            writer.writeheader()
            writer.writerows(event_log)
        print(f"[INFO] Event log disimpan ke {event_file} ({len(event_log)} event)")

    return filepath


def create_plots(data_list, event_log, filename):
    """Membuat grafik analisis data LCD."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        print("[WARN] matplotlib tidak tersedia. Grafik tidak dibuat.")
        print("       Install dengan: pip install matplotlib")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Analisis LCD PCF8574 I2C - ESP32', fontsize=14, fontweight='bold')

    # Plot 1: Counter vs Waktu
    ax1 = axes[0, 0]
    if data_list:
        times = [d['timestamp'] for d in data_list]
        counters = [d['counter'] for d in data_list]
        ax1.plot(times, counters, 'b-o', markersize=3, linewidth=1)
        ax1.set_xlabel('Waktu (detik)')
        ax1.set_ylabel('Nilai Counter')
        ax1.set_title('Counter LCD vs Waktu')
        ax1.grid(True, alpha=0.3)

        # Tandai event backlight
        if event_log:
            for evt in event_log:
                ax1.axvline(x=evt['timestamp'], color='orange', linestyle='--',
                            alpha=0.7, label='Backlight Toggle')
    else:
        ax1.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax1.transAxes)

    # Plot 2: Interval Update
    ax2 = axes[0, 1]
    if len(data_list) > 1:
        intervals = []
        for i in range(1, len(data_list)):
            dt = data_list[i]['timestamp'] - data_list[i-1]['timestamp']
            intervals.append(dt)
        ax2.plot(range(len(intervals)), intervals, 'g-', linewidth=1)
        ax2.axhline(y=1.0, color='red', linestyle='--', alpha=0.5, label='Target 1.0s')
        if intervals:
            avg_int = sum(intervals) / len(intervals)
            ax2.axhline(y=avg_int, color='blue', linestyle='--', alpha=0.5,
                        label=f'Rata-rata: {avg_int:.3f}s')
        ax2.set_xlabel('Sampel')
        ax2.set_ylabel('Interval (detik)')
        ax2.set_title('Interval Antar-Update LCD')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
    else:
        ax2.text(0.5, 0.5, 'Data tidak cukup', ha='center', va='center',
                 transform=ax2.transAxes)

    # Plot 3: Histogram Interval
    ax3 = axes[1, 0]
    if len(data_list) > 2:
        intervals = []
        for i in range(1, len(data_list)):
            dt = data_list[i]['timestamp'] - data_list[i-1]['timestamp']
            intervals.append(dt)
        ax3.hist(intervals, bins=20, color='steelblue', edgecolor='white', alpha=0.8)
        ax3.set_xlabel('Interval (detik)')
        ax3.set_ylabel('Frekuensi')
        ax3.set_title('Distribusi Interval Update')
        ax3.grid(True, alpha=0.3)
    else:
        ax3.text(0.5, 0.5, 'Data tidak cukup', ha='center', va='center',
                 transform=ax3.transAxes)

    # Plot 4: Timeline Event
    ax4 = axes[1, 1]
    if data_list:
        times = [d['timestamp'] for d in data_list]
        ax4.barh(['Counter Update'], [len(data_list)], color='steelblue', label='Updates')
        if event_log:
            ax4.barh(['Backlight Toggle'], [len(event_log)], color='orange',
                     label='Toggles')
        ax4.set_xlabel('Jumlah Event')
        ax4.set_title('Ringkasan Event')
        ax4.legend()

        # Info teks
        total_time = times[-1] - times[0] if len(times) > 1 else 0
        ax4.text(0.5, -0.2, f'Total waktu: {total_time:.1f}s | '
                 f'Updates: {len(data_list)} | Events: {len(event_log)}',
                 ha='center', va='center', transform=ax4.transAxes, fontsize=10)
    else:
        ax4.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax4.transAxes)

    plt.tight_layout()
    filepath = os.path.join(os.path.dirname(__file__), filename)
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"[INFO] Grafik disimpan ke {filepath}")


def print_summary(data_list, event_log):
    """Menampilkan ringkasan analisis."""
    print("\n" + "=" * 60)
    print("          RINGKASAN ANALISIS LCD PCF8574")
    print("=" * 60)

    if data_list:
        counters = [d['counter'] for d in data_list]
        times = [d['timestamp'] for d in data_list]
        print(f"\n  Statistik Counter:")
        print(f"    Total update     : {len(data_list)}")
        print(f"    Counter awal     : {counters[0]}")
        print(f"    Counter akhir    : {counters[-1]}")
        print(f"    Durasi total     : {times[-1] - times[0]:.1f} detik")

        if len(data_list) > 1:
            intervals = [times[i] - times[i-1] for i in range(1, len(times))]
            print(f"    Interval rata-rata: {sum(intervals)/len(intervals):.3f} detik")
            print(f"    Interval min      : {min(intervals):.3f} detik")
            print(f"    Interval max      : {max(intervals):.3f} detik")

    if event_log:
        print(f"\n  Event Backlight Toggle: {len(event_log)} kali")

    print("=" * 60)


def main():
    """Fungsi utama program."""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    print("=" * 60)
    print("  ESP32 LCD PCF8574 I2C - Debug Analysis")
    print(f"  Port: {port} | Durasi: {duration}s")
    print("=" * 60)

    data_list, event_log = parse_serial_data(port, duration)

    if not data_list:
        print("[WARN] Tidak ada data yang terkumpul.")
        return

    save_to_csv(data_list, event_log, CSV_FILENAME)
    create_plots(data_list, event_log, PLOT_FILENAME)
    print_summary(data_list, event_log)


if __name__ == '__main__':
    main()

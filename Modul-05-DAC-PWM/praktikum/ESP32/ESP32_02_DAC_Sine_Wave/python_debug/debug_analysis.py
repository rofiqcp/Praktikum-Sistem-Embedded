#!/usr/bin/env python3
"""
Debug & Analysis Script - DAC Sine Wave
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data frekuensi gelombang sinus dari ESP32 dan memvisualisasikan.
"""

import serial
import time
import csv
import argparse
import re

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib/numpy tidak terinstal. Plotting dinonaktifkan.")
    print("Install dengan: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='DAC Sine Wave - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='dac_sine_wave.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<total_siklus>,<frekuensi_aktual>,<indeks>"""
    match = re.search(r'DATA:(\d+),(\d+),(\d+)', line)
    if match:
        return {
            'total_cycles': int(match.group(1)),
            'frequency': int(match.group(2)),
            'index': int(match.group(3))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print(f"[INFO] Menyimpan ke: {output_file}")
    print("-" * 60)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'total_cycles', 'frequency_hz', 'index'])

        while (time.time() - start_time) < duration:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  {line}")
                        parsed = parse_line(line)
                        if parsed:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            data.append(parsed)
                            writer.writerow([
                                f"{elapsed:.3f}",
                                parsed['total_cycles'],
                                parsed['frequency'],
                                parsed['index']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi data gelombang sinus."""
    if not HAS_MATPLOTLIB or not data:
        return

    freqs = [d['frequency'] for d in data]
    cycles = [d['total_cycles'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('DAC Sine Wave - Analisis', fontsize=14)

    # Plot frekuensi aktual vs waktu
    ax1.plot(timestamps, freqs, 'b-o', markersize=4)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Frekuensi (Hz)')
    ax1.set_title('Frekuensi Aktual Gelombang Sinus')
    ax1.grid(True, alpha=0.3)
    if freqs:
        avg_freq = np.mean(freqs)
        ax1.axhline(y=avg_freq, color='r', linestyle='--', alpha=0.5,
                     label=f'Rata-rata: {avg_freq:.1f} Hz')
        ax1.legend()

    # Plot total siklus vs waktu
    ax2.plot(timestamps, cycles, 'g-o', markersize=4)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Total Siklus')
    ax2.set_title('Jumlah Siklus Kumulatif')
    ax2.grid(True, alpha=0.3)

    # Tambahkan plot gelombang sinus referensi
    fig2, ax3 = plt.subplots(figsize=(12, 4))
    t = np.linspace(0, 2 * np.pi * 3, 768)
    sine_wave = (np.sin(t) + 1) * 127.5
    ax3.plot(t, sine_wave, 'b-')
    ax3.set_xlabel('Fase (rad)')
    ax3.set_ylabel('Nilai DAC (0-255)')
    ax3.set_title('Bentuk Gelombang Sinus Referensi (3 siklus)')
    ax3.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('dac_sine_wave.png', dpi=150)
    print("[INFO] Grafik disimpan: dac_sine_wave.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  DAC Sine Wave - Debug & Analysis Tool")
    print("  Modul 05 - DAC & PWM")
    print("=" * 60)
    print(f"  Port     : {args.port}")
    print(f"  Baud     : {args.baud}")
    print(f"  Durasi   : {args.duration} detik")
    print(f"  Output   : {args.output}")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            freqs = [d['frequency'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Frekuensi rata-rata : {np.mean(freqs):.1f} Hz" if HAS_MATPLOTLIB else "")
            print(f"  Frekuensi min       : {min(freqs)} Hz")
            print(f"  Frekuensi max       : {max(freqs)} Hz")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()

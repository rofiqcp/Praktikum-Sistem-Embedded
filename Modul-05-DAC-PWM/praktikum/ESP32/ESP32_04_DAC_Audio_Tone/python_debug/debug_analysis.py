#!/usr/bin/env python3
"""
Debug & Analysis Script - DAC Audio Tone
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data nada audio dari ESP32 dan memvisualisasikan pola nada.
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
        description='DAC Audio Tone - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='dac_audio_tone.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<note>,<freq>,<iteration>"""
    match = re.search(r'DATA:(\w+),(\d+),(\d+)', line)
    if match:
        return {
            'note': match.group(1),
            'frequency': int(match.group(2)),
            'iteration': int(match.group(3))
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
        writer.writerow(['timestamp', 'note', 'frequency', 'iteration'])

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
                                parsed['note'],
                                parsed['frequency'],
                                parsed['iteration']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi pola nada audio."""
    if not HAS_MATPLOTLIB or not data:
        return

    freqs = [d['frequency'] for d in data]
    notes = [d['note'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('DAC Audio Tone - Analisis', fontsize=14)

    # Plot frekuensi vs waktu
    colors = ['blue' if n == 'A4' else ('red' if n == 'A5' else 'gray') for n in notes]
    ax1.scatter(timestamps, freqs, c=colors, s=50, zorder=5)
    ax1.step(timestamps, freqs, 'k-', alpha=0.3, where='post')
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Frekuensi (Hz)')
    ax1.set_title('Pola Nada Audio')
    ax1.grid(True, alpha=0.3)
    ax1.axhline(y=440, color='blue', linestyle='--', alpha=0.3, label='A4 (440 Hz)')
    ax1.axhline(y=880, color='red', linestyle='--', alpha=0.3, label='A5 (880 Hz)')
    ax1.legend()

    # Gelombang sinus referensi
    t = np.linspace(0, 0.01, 1000)
    sine_440 = np.sin(2 * np.pi * 440 * t)
    sine_880 = np.sin(2 * np.pi * 880 * t)
    ax2.plot(t * 1000, sine_440, 'b-', label='A4 (440 Hz)', alpha=0.7)
    ax2.plot(t * 1000, sine_880, 'r-', label='A5 (880 Hz)', alpha=0.7)
    ax2.set_xlabel('Waktu (ms)')
    ax2.set_ylabel('Amplitudo')
    ax2.set_title('Bentuk Gelombang Referensi (10ms)')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('dac_audio_tone.png', dpi=150)
    print("[INFO] Grafik disimpan: dac_audio_tone.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  DAC Audio Tone - Debug & Analysis Tool")
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
            notes = [d['note'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Total event      : {len(data)}")
            print(f"  Nada A4 dimainkan : {notes.count('A4')} kali")
            print(f"  Nada A5 dimainkan : {notes.count('A5')} kali")
            print(f"  Senyap           : {notes.count('SILENT')} kali")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()

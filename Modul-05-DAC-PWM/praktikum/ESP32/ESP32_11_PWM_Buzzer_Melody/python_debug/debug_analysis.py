#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM Buzzer Melody
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data nada melodi dari ESP32 dan memvisualisasikan partitur.
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
        description='PWM Buzzer Melody - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data dalam detik (default: 60)')
    parser.add_argument('-o', '--output', default='pwm_buzzer_melody.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<indeks>,<nama>,<freq>,<durasi>,<pemutaran>"""
    match = re.search(r'DATA:(\d+),(\w+),(\d+),(\d+),(\d+)', line)
    if match:
        return {
            'index': int(match.group(1)),
            'note_name': match.group(2),
            'frequency': int(match.group(3)),
            'duration': int(match.group(4)),
            'play_count': int(match.group(5))
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
        writer.writerow(['timestamp', 'index', 'note_name', 'frequency', 'duration_ms', 'play_count'])

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
                                parsed['index'],
                                parsed['note_name'],
                                parsed['frequency'],
                                parsed['duration'],
                                parsed['play_count']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi melodi sebagai partitur sederhana."""
    if not HAS_MATPLOTLIB or not data:
        return

    freqs = [d['frequency'] for d in data]
    names = [d['note_name'] for d in data]
    durations = [d['duration'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8))
    fig.suptitle('PWM Buzzer Melody - Analisis Partitur', fontsize=14)

    # Plot frekuensi (nada) vs waktu - visualisasi partitur
    for i in range(len(data)):
        if freqs[i] > 0:
            dur_sec = durations[i] / 1000.0
            ax1.barh(freqs[i], dur_sec, left=timestamps[i],
                     height=20, color='steelblue', edgecolor='navy', alpha=0.7)
            ax1.text(timestamps[i] + dur_sec / 2, freqs[i] + 15,
                     names[i], ha='center', va='bottom', fontsize=7)

    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Frekuensi (Hz)')
    ax1.set_title('Partitur Melodi - Frekuensi vs Waktu')
    ax1.grid(True, alpha=0.3, axis='x')

    # Peta nada
    note_map = {'C4': 262, 'D4': 294, 'E4': 330, 'F4': 349,
                'G4': 392, 'A4': 440, 'B4': 494, 'C5': 523}
    for name, freq in note_map.items():
        ax1.axhline(y=freq, color='gray', linestyle=':', alpha=0.3)
        ax1.text(-0.5, freq, name, va='center', fontsize=8, color='gray')

    # Histogram distribusi nada
    note_counts = {}
    for d in data:
        if d['frequency'] > 0:
            name = d['note_name']
            note_counts[name] = note_counts.get(name, 0) + 1

    if note_counts:
        sorted_notes = sorted(note_counts.items(),
                              key=lambda x: {'C4': 1, 'D4': 2, 'E4': 3, 'F4': 4,
                                             'G4': 5, 'A4': 6, 'B4': 7, 'C5': 8}.get(x[0], 9))
        note_names = [n[0] for n in sorted_notes]
        counts = [n[1] for n in sorted_notes]
        colors = plt.cm.rainbow(np.linspace(0, 1, len(note_names)))
        ax2.bar(note_names, counts, color=colors)
        ax2.set_xlabel('Nada')
        ax2.set_ylabel('Jumlah Kemunculan')
        ax2.set_title('Distribusi Nada dalam Melodi')
        ax2.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig('pwm_buzzer_melody.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_buzzer_melody.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM Buzzer Melody - Debug & Analysis Tool")
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
            print(f"\n[STATISTIK]")
            print(f"  Total nada        : {len(data)}")
            print(f"  Total pemutaran   : {data[-1]['play_count']}")
            unique_notes = set(d['note_name'] for d in data if d['frequency'] > 0)
            print(f"  Nada unik         : {', '.join(sorted(unique_notes))}")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()

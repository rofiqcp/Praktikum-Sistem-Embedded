#!/usr/bin/env python3
"""
Debug & Analisis - ESP32_02_I2C_OLED_SSD1306
Modul 06 - I2C & Sensor

Membaca output serial dari program OLED SSD1306,
mencatat update counter ke CSV dan menampilkan grafik.
"""

import argparse
import csv
import re
import sys
from datetime import datetime

import serial
import matplotlib.pyplot as plt


def parse_args():
    """Parsing argumen baris perintah."""
    parser = argparse.ArgumentParser(
        description="Analisis output OLED SSD1306 ESP32"
    )
    parser.add_argument(
        "--port", "-p",
        default="/dev/ttyUSB0",
        help="Port serial (default: /dev/ttyUSB0)"
    )
    parser.add_argument(
        "--baud", "-b",
        type=int,
        default=115200,
        help="Baud rate (default: 115200)"
    )
    parser.add_argument(
        "--output", "-o",
        default="oled_log.csv",
        help="File output CSV (default: oled_log.csv)"
    )
    parser.add_argument(
        "--duration", "-d",
        type=int,
        default=60,
        help="Durasi pemantauan dalam detik (default: 60)"
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Buka koneksi serial
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print(f"[INFO] Terhubung ke {args.port} @ {args.baud} baud")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        sys.exit(1)

    # Siapkan file CSV
    csv_file = open(args.output, "w", newline="")
    writer = csv.writer(csv_file)
    writer.writerow(["timestamp", "counter"])

    # Pola regex
    pattern = re.compile(r"OLED_UPDATE:\s*counter=(\d+)")

    counters = []
    timestamps = []

    print("[INFO] Memulai pemantauan... (Ctrl+C untuk berhenti)")
    start_time = datetime.now()

    try:
        while True:
            elapsed = (datetime.now() - start_time).total_seconds()
            if elapsed > args.duration:
                print(f"[INFO] Durasi {args.duration}s tercapai.")
                break

            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue

            m = pattern.search(line)
            if m:
                counter = int(m.group(1))
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                writer.writerow([ts, counter])
                counters.append(counter)
                timestamps.append(elapsed)
                print(f"  [OLED] Counter: {counter}")

    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        csv_file.close()
        ser.close()

    # Ringkasan
    print(f"\n{'='*40}")
    print("RINGKASAN OLED SSD1306")
    print(f"{'='*40}")
    print(f"Total update : {len(counters)}")
    if counters:
        print(f"Counter awal : {counters[0]}")
        print(f"Counter akhir: {counters[-1]}")
    print(f"Data disimpan: {args.output}")

    # Grafik
    if counters:
        fig, ax = plt.subplots(figsize=(10, 5))
        ax.plot(timestamps, counters, marker=".", color="teal")
        ax.set_xlabel("Waktu (detik)")
        ax.set_ylabel("Nilai Counter")
        ax.set_title("OLED SSD1306 - Update Counter")
        ax.grid(True, alpha=0.3)
        plt.tight_layout()
        plot_file = args.output.replace(".csv", "_plot.png")
        plt.savefig(plot_file, dpi=150)
        print(f"Grafik disimpan: {plot_file}")
        plt.show()


if __name__ == "__main__":
    main()

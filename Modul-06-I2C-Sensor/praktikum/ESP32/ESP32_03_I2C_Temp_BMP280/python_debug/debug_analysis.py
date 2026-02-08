#!/usr/bin/env python3
"""
Debug & Analisis - ESP32_03_I2C_Temp_BMP280
Modul 06 - I2C & Sensor

Membaca data suhu dan tekanan dari serial,
menyimpan ke CSV, dan membuat grafik real-time.
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
        description="Analisis output sensor BMP280 ESP32"
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
        default="bmp280_log.csv",
        help="File output CSV (default: bmp280_log.csv)"
    )
    parser.add_argument(
        "--duration", "-d",
        type=int,
        default=120,
        help="Durasi pemantauan dalam detik (default: 120)"
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Buka koneksi serial
    try:
        ser = serial.Serial(args.port, args.baud, timeout=2)
        print(f"[INFO] Terhubung ke {args.port} @ {args.baud} baud")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        sys.exit(1)

    # Siapkan file CSV
    csv_file = open(args.output, "w", newline="")
    writer = csv.writer(csv_file)
    writer.writerow(["timestamp", "sample", "suhu_C", "tekanan_hPa"])

    # Pola regex untuk parsing
    pattern = re.compile(
        r"BMP280_DATA:\s*sample=(\d+)\s+temp=([0-9.NaN-]+)\s+press=([0-9.NaN-]+)"
    )

    temps = []
    presses = []
    times = []

    print("[INFO] Memulai pemantauan BMP280... (Ctrl+C untuk berhenti)")
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
                sample = int(m.group(1))
                temp_str = m.group(2)
                press_str = m.group(3)

                try:
                    temp = float(temp_str)
                    press = float(press_str)
                except ValueError:
                    temp = float("nan")
                    press = float("nan")

                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                writer.writerow([ts, sample, temp, press])
                temps.append(temp)
                presses.append(press)
                times.append(elapsed)
                print(f"  [BMP280] #{sample} Suhu={temp:.2f}°C  Tekanan={press:.2f}hPa")

    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        csv_file.close()
        ser.close()

    # Ringkasan
    valid_temps = [t for t in temps if t == t]  # Filter NaN
    valid_press = [p for p in presses if p == p]

    print(f"\n{'='*45}")
    print("RINGKASAN PEMBACAAN BMP280")
    print(f"{'='*45}")
    print(f"Total sampel       : {len(temps)}")
    if valid_temps:
        print(f"Suhu rata-rata     : {sum(valid_temps)/len(valid_temps):.2f} °C")
        print(f"Suhu min/maks      : {min(valid_temps):.2f} / {max(valid_temps):.2f} °C")
    if valid_press:
        print(f"Tekanan rata-rata  : {sum(valid_press)/len(valid_press):.2f} hPa")
        print(f"Tekanan min/maks   : {min(valid_press):.2f} / {max(valid_press):.2f} hPa")
    print(f"Data disimpan ke   : {args.output}")

    # Grafik
    if valid_temps and valid_press:
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

        # Grafik suhu
        ax1.plot(times, temps, color="red", marker=".", markersize=4)
        ax1.set_ylabel("Suhu (°C)")
        ax1.set_title("BMP280 - Pembacaan Suhu & Tekanan")
        ax1.grid(True, alpha=0.3)

        # Grafik tekanan
        ax2.plot(times, presses, color="blue", marker=".", markersize=4)
        ax2.set_xlabel("Waktu (detik)")
        ax2.set_ylabel("Tekanan (hPa)")
        ax2.grid(True, alpha=0.3)

        plt.tight_layout()
        plot_file = args.output.replace(".csv", "_plot.png")
        plt.savefig(plot_file, dpi=150)
        print(f"Grafik disimpan ke : {plot_file}")
        plt.show()


if __name__ == "__main__":
    main()

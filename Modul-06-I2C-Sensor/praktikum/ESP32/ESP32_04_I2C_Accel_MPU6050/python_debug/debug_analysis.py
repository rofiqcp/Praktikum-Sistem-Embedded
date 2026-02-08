#!/usr/bin/env python3
"""
Debug & Analisis - ESP32_04_I2C_Accel_MPU6050
Modul 06 - I2C & Sensor

Membaca data akselerometer dan giroskop dari serial,
menyimpan ke CSV, dan menampilkan grafik multi-axis.
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
        description="Analisis output sensor MPU6050 ESP32"
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
        default="mpu6050_log.csv",
        help="File output CSV (default: mpu6050_log.csv)"
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
    writer.writerow(["timestamp", "sample", "ax", "ay", "az", "gx", "gy", "gz", "temp"])

    # Pola regex untuk parsing
    pattern = re.compile(
        r"MPU6050_DATA:\s*sample=(\d+)\s+"
        r"ax=([0-9.e+-]+)\s+ay=([0-9.e+-]+)\s+az=([0-9.e+-]+)\s+"
        r"gx=([0-9.e+-]+)\s+gy=([0-9.e+-]+)\s+gz=([0-9.e+-]+)\s+"
        r"temp=([0-9.e+-]+)"
    )

    # Penyimpanan data
    data = {"ax": [], "ay": [], "az": [],
            "gx": [], "gy": [], "gz": [],
            "temp": [], "time": []}

    print("[INFO] Memulai pemantauan MPU6050... (Ctrl+C untuk berhenti)")
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
                vals = [float(m.group(i)) for i in range(2, 9)]
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                writer.writerow([ts, sample] + vals)

                data["ax"].append(vals[0])
                data["ay"].append(vals[1])
                data["az"].append(vals[2])
                data["gx"].append(vals[3])
                data["gy"].append(vals[4])
                data["gz"].append(vals[5])
                data["temp"].append(vals[6])
                data["time"].append(elapsed)

                print(f"  [MPU] #{sample} A=({vals[0]:.2f},{vals[1]:.2f},{vals[2]:.2f})g "
                      f"G=({vals[3]:.1f},{vals[4]:.1f},{vals[5]:.1f})°/s T={vals[6]:.1f}°C")

    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        csv_file.close()
        ser.close()

    # Ringkasan
    n = len(data["ax"])
    print(f"\n{'='*50}")
    print("RINGKASAN PEMBACAAN MPU6050")
    print(f"{'='*50}")
    print(f"Total sampel: {n}")
    if n > 0:
        for axis in ["ax", "ay", "az"]:
            avg = sum(data[axis]) / n
            print(f"  {axis} rata-rata: {avg:.4f} g")
        for axis in ["gx", "gy", "gz"]:
            avg = sum(data[axis]) / n
            print(f"  {axis} rata-rata: {avg:.2f} °/s")
    print(f"Data disimpan ke: {args.output}")

    # Grafik
    if n > 0:
        fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)

        # Grafik akselerometer
        axes[0].plot(data["time"], data["ax"], label="X", color="red")
        axes[0].plot(data["time"], data["ay"], label="Y", color="green")
        axes[0].plot(data["time"], data["az"], label="Z", color="blue")
        axes[0].set_ylabel("Akselerasi (g)")
        axes[0].set_title("MPU6050 - Data Akselerometer & Giroskop")
        axes[0].legend()
        axes[0].grid(True, alpha=0.3)

        # Grafik giroskop
        axes[1].plot(data["time"], data["gx"], label="X", color="red")
        axes[1].plot(data["time"], data["gy"], label="Y", color="green")
        axes[1].plot(data["time"], data["gz"], label="Z", color="blue")
        axes[1].set_ylabel("Kec. Sudut (°/s)")
        axes[1].legend()
        axes[1].grid(True, alpha=0.3)

        # Grafik suhu
        axes[2].plot(data["time"], data["temp"], color="orange")
        axes[2].set_xlabel("Waktu (detik)")
        axes[2].set_ylabel("Suhu (°C)")
        axes[2].grid(True, alpha=0.3)

        plt.tight_layout()
        plot_file = args.output.replace(".csv", "_plot.png")
        plt.savefig(plot_file, dpi=150)
        print(f"Grafik disimpan ke: {plot_file}")
        plt.show()


if __name__ == "__main__":
    main()

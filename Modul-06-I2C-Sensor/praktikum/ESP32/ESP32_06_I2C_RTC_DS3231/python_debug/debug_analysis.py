#!/usr/bin/env python3
"""
Debug & Analisis - ESP32_06_I2C_RTC_DS3231
Modul 06 - I2C & Sensor

Membaca data waktu dan suhu dari serial DS3231,
menyimpan ke CSV, dan menampilkan grafik suhu serta timeline.
"""

import argparse
import csv
import re
import sys
from datetime import datetime

import serial
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


def parse_args():
    """Parsing argumen baris perintah."""
    parser = argparse.ArgumentParser(
        description="Analisis output RTC DS3231 ESP32"
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
        default="ds3231_log.csv",
        help="File output CSV (default: ds3231_log.csv)"
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
    writer.writerow(["timestamp_lokal", "sample", "tanggal_rtc", "waktu_rtc",
                      "hari", "suhu_C", "alarm"])

    # Pola regex untuk parsing
    pattern = re.compile(
        r"RTC_TIME:\s*sample=(\d+)\s+"
        r"date=(\d{4}-\d{2}-\d{2})\s+"
        r"time=(\d{2}:\d{2}:\d{2})\s+"
        r"day=(\S+)"
        r"(?:\s+temp=([0-9.e+-]+))?"
        r"(?:\s+ALARM=1)?"
    )
    alarm_pattern = re.compile(r"ALARM=1")

    # Penyimpanan data
    temps = []
    times_elapsed = []
    rtc_times = []
    alarms = []

    print("[INFO] Memulai pemantauan DS3231... (Ctrl+C untuk berhenti)")
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
                rtc_date = m.group(2)
                rtc_time = m.group(3)
                day = m.group(4)
                temp_str = m.group(5)
                has_alarm = bool(alarm_pattern.search(line))

                temp = float(temp_str) if temp_str else float("nan")
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                writer.writerow([ts, sample, rtc_date, rtc_time, day,
                                 f"{temp:.2f}", "YA" if has_alarm else ""])

                temps.append(temp)
                times_elapsed.append(elapsed)
                rtc_times.append(f"{rtc_date} {rtc_time}")
                alarms.append(has_alarm)

                alarm_str = " *** ALARM! ***" if has_alarm else ""
                print(f"  [RTC] {day} {rtc_date} {rtc_time} | "
                      f"Suhu: {temp:.2f}°C{alarm_str}")

    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        csv_file.close()
        ser.close()

    # Ringkasan
    valid_temps = [t for t in temps if t == t]
    alarm_count = sum(alarms)

    print(f"\n{'='*45}")
    print("RINGKASAN PEMBACAAN DS3231 RTC")
    print(f"{'='*45}")
    print(f"Total pembacaan  : {len(temps)}")
    if rtc_times:
        print(f"Waktu RTC awal   : {rtc_times[0]}")
        print(f"Waktu RTC akhir  : {rtc_times[-1]}")
    if valid_temps:
        print(f"Suhu rata-rata   : {sum(valid_temps)/len(valid_temps):.2f} °C")
        print(f"Suhu min/maks    : {min(valid_temps):.2f} / {max(valid_temps):.2f} °C")
    print(f"Alarm terpicu    : {alarm_count}x")
    print(f"Data disimpan ke : {args.output}")

    # Grafik
    if valid_temps:
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

        # Grafik suhu
        ax1.plot(times_elapsed, temps, color="red", marker=".", markersize=3)
        ax1.set_ylabel("Suhu (°C)")
        ax1.set_title("DS3231 RTC - Suhu Internal & Alarm")
        ax1.grid(True, alpha=0.3)

        # Tandai alarm
        for i, has_alarm in enumerate(alarms):
            if has_alarm:
                ax1.axvline(x=times_elapsed[i], color="orange",
                            linestyle="--", alpha=0.7, label="Alarm" if i == 0 else "")

        if alarm_count > 0:
            ax1.legend()

        # Grafik drift (perbedaan waktu RTC vs waktu lokal)
        if len(times_elapsed) > 1:
            # Hitung interval antar pembacaan
            intervals = []
            for i in range(1, len(times_elapsed)):
                dt = times_elapsed[i] - times_elapsed[i-1]
                intervals.append(dt)
            ax2.plot(range(1, len(intervals) + 1), intervals,
                     color="blue", marker=".", markersize=3)
            ax2.axhline(y=1.0, color="green", linestyle="--",
                        alpha=0.5, label="Interval ideal (1s)")
            ax2.set_xlabel("Pembacaan ke-")
            ax2.set_ylabel("Interval (detik)")
            ax2.set_title("Interval Pembacaan RTC")
            ax2.legend()
            ax2.grid(True, alpha=0.3)

        plt.tight_layout()
        plot_file = args.output.replace(".csv", "_plot.png")
        plt.savefig(plot_file, dpi=150)
        print(f"Grafik disimpan ke: {plot_file}")
        plt.show()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Debug & Analisis - ESP32_05_I2C_EEPROM_AT24C32
Modul 06 - I2C & Sensor

Membaca hasil tes EEPROM dari serial,
mencatat hasil pass/fail ke CSV, dan menampilkan ringkasan.
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
        description="Analisis output EEPROM AT24C32 ESP32"
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
        default="eeprom_log.csv",
        help="File output CSV (default: eeprom_log.csv)"
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
    writer.writerow(["timestamp", "test_name", "pass", "fail"])

    # Pola regex untuk parsing
    test_pattern = re.compile(r"EEPROM_TEST(\d+):\s*pass=(\d+)\s+fail=(\d+)")
    cycle_pattern = re.compile(r"EEPROM_CYCLE:\s*cycle=(\d+)")

    # Statistik
    test_results = {"TEST1": [], "TEST2": [], "TEST3": []}
    cycles = []

    print("[INFO] Memulai pemantauan EEPROM... (Ctrl+C untuk berhenti)")
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

            # Cek hasil tes
            m_test = test_pattern.search(line)
            if m_test:
                test_num = m_test.group(1)
                pass_count = int(m_test.group(2))
                fail_count = int(m_test.group(3))
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                test_name = f"TEST{test_num}"
                writer.writerow([ts, test_name, pass_count, fail_count])
                test_results[test_name].append(
                    {"pass": pass_count, "fail": fail_count}
                )
                status = "LULUS" if fail_count == 0 else "GAGAL"
                print(f"  [{test_name}] {status} - pass={pass_count} fail={fail_count}")

            # Cek siklus selesai
            m_cycle = cycle_pattern.search(line)
            if m_cycle:
                cycle = int(m_cycle.group(1))
                cycles.append(cycle)
                print(f"  [SIKLUS] #{cycle} selesai")

    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        csv_file.close()
        ser.close()

    # Ringkasan
    print(f"\n{'='*45}")
    print("RINGKASAN TES EEPROM AT24C32")
    print(f"{'='*45}")
    print(f"Total siklus: {len(cycles)}")
    for test_name, results in test_results.items():
        if results:
            total_pass = sum(r["pass"] for r in results)
            total_fail = sum(r["fail"] for r in results)
            print(f"  {test_name}: {total_pass} lulus, {total_fail} gagal")
    print(f"Data disimpan ke: {args.output}")

    # Grafik
    if cycles:
        fig, ax = plt.subplots(figsize=(10, 6))

        test_names = ["TEST1", "TEST2", "TEST3"]
        labels = ["Byte R/W", "Page R/W", "String R/W"]
        colors = ["#2ecc71", "#3498db", "#e74c3c"]

        for i, (tn, label) in enumerate(zip(test_names, labels)):
            results = test_results[tn]
            if results:
                pass_vals = [r["pass"] for r in results]
                ax.plot(range(1, len(pass_vals) + 1), pass_vals,
                        marker="o", label=f"{label} (Pass)", color=colors[i])

        ax.set_xlabel("Siklus ke-")
        ax.set_ylabel("Jumlah Test Lulus")
        ax.set_title("EEPROM AT24C32 - Hasil Test per Siklus")
        ax.legend()
        ax.grid(True, alpha=0.3)

        plt.tight_layout()
        plot_file = args.output.replace(".csv", "_plot.png")
        plt.savefig(plot_file, dpi=150)
        print(f"Grafik disimpan ke: {plot_file}")
        plt.show()


if __name__ == "__main__":
    main()

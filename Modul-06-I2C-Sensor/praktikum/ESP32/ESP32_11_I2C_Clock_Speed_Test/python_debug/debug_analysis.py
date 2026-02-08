"""
ESP32_11_I2C_Clock_Speed_Test - Debug Analysis
Modul 06 - I2C & Sensor

Deskripsi: Parser serial + analisis perbandingan kecepatan I2C.
           Membandingkan throughput 100kHz vs 400kHz.
           Grafik perbandingan throughput, latency, dan efisiensi.

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

# Konfigurasi default
DEFAULT_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILENAME = 'i2c_speed_data.csv'
PLOT_FILENAME = 'i2c_speed_plot.png'
MAX_SAMPLES = 100

def parse_serial_data(port, duration=120):
    """Membaca dan mem-parsing data serial pengujian kecepatan I2C."""
    speed_results = []
    speedup_data = []

    print(f"[INFO] Membuka port serial {port} pada {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(2)
        ser.reset_input_buffer()
        print(f"[INFO] Port serial terbuka. Membaca data selama {duration} detik...")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti lebih awal.\n")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        return speed_results, speedup_data

    start_time = time.time()
    sample_count = 0

    try:
        while (time.time() - start_time) < duration and sample_count < MAX_SAMPLES:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue

                print(f"  RAW: {line}")
                timestamp = time.time() - start_time

                # Parse: RESULT,speed_hz,total_bytes,total_time_us,throughput_bps,
                #        avg_latency_us,success,fail
                if line.startswith('RESULT,'):
                    parts = line.split(',')
                    if len(parts) >= 8:
                        try:
                            entry = {
                                'timestamp': timestamp,
                                'speed_hz': int(parts[1]),
                                'total_bytes': int(parts[2]),
                                'total_time_us': int(parts[3]),
                                'throughput_bps': float(parts[4]),
                                'avg_latency_us': float(parts[5]),
                                'success': int(parts[6]),
                                'fail': int(parts[7])
                            }
                            speed_results.append(entry)
                            sample_count += 1

                            speed_khz = entry['speed_hz'] / 1000
                            tp_kbps = entry['throughput_bps'] / 1024
                            print(f"  [RESULT] {speed_khz:.0f}kHz: "
                                  f"{tp_kbps:.2f} KB/s, "
                                  f"Latency: {entry['avg_latency_us']:.0f}μs, "
                                  f"OK: {entry['success']}, FAIL: {entry['fail']}")
                        except (ValueError, IndexError) as e:
                            print(f"  [WARN] Gagal parse RESULT: {e}")

                # Parse: SPEEDUP,ratio,efficiency
                elif line.startswith('SPEEDUP,'):
                    parts = line.split(',')
                    if len(parts) >= 3:
                        try:
                            speedup_data.append({
                                'timestamp': timestamp,
                                'ratio': float(parts[1]),
                                'efficiency': float(parts[2])
                            })
                            print(f"  [SPEEDUP] Rasio: {parts[1]}x, "
                                  f"Efisiensi: {parts[2]}%")
                        except (ValueError, IndexError):
                            pass

    except KeyboardInterrupt:
        print("\n[INFO] Pembacaan dihentikan oleh pengguna.")
    finally:
        ser.close()
        print(f"[INFO] Port serial ditutup. Total sampel: {sample_count}")

    return speed_results, speedup_data


def save_to_csv(speed_results, filename):
    """Menyimpan data ke file CSV."""
    filepath = os.path.join(os.path.dirname(__file__), filename)
    with open(filepath, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile,
                                fieldnames=['timestamp', 'speed_hz', 'total_bytes',
                                            'total_time_us', 'throughput_bps',
                                            'avg_latency_us', 'success', 'fail'])
        writer.writeheader()
        writer.writerows(speed_results)

    print(f"[INFO] Data disimpan ke {filepath} ({len(speed_results)} baris)")
    return filepath


def create_plots(speed_results, speedup_data, filename):
    """Membuat grafik perbandingan kecepatan I2C."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        print("[WARN] matplotlib tidak tersedia. Grafik tidak dibuat.")
        print("       Install dengan: pip install matplotlib")
        return

    if not speed_results:
        print("[WARN] Tidak ada data untuk grafik.")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Perbandingan Kecepatan I2C (100kHz vs 400kHz) - ESP32',
                 fontsize=14, fontweight='bold')

    # Kelompokkan data berdasarkan kecepatan
    speed_groups = {}
    for r in speed_results:
        spd = r['speed_hz']
        if spd not in speed_groups:
            speed_groups[spd] = []
        speed_groups[spd].append(r)

    speeds = sorted(speed_groups.keys())
    speed_labels = [f"{s//1000}kHz" for s in speeds]

    # Plot 1: Throughput Perbandingan (Bar Chart)
    ax1 = axes[0, 0]
    throughputs = []
    for spd in speeds:
        avg_tp = sum(r['throughput_bps'] for r in speed_groups[spd]) / len(speed_groups[spd])
        throughputs.append(avg_tp / 1024)  # Konversi ke KB/s

    colors_bar = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12']
    bars = ax1.bar(speed_labels, throughputs, color=colors_bar[:len(speeds)], alpha=0.8)
    ax1.set_xlabel('Kecepatan I2C')
    ax1.set_ylabel('Throughput (KB/s)')
    ax1.set_title('Throughput per Kecepatan')
    ax1.grid(True, alpha=0.3, axis='y')
    for bar, tp in zip(bars, throughputs):
        ax1.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                 f'{tp:.2f}', ha='center', va='bottom', fontweight='bold')

    # Plot 2: Latency Perbandingan (Bar Chart)
    ax2 = axes[0, 1]
    latencies = []
    for spd in speeds:
        avg_lat = sum(r['avg_latency_us'] for r in speed_groups[spd]) / len(speed_groups[spd])
        latencies.append(avg_lat)

    bars2 = ax2.bar(speed_labels, latencies, color=colors_bar[:len(speeds)], alpha=0.8)
    ax2.set_xlabel('Kecepatan I2C')
    ax2.set_ylabel('Latency Rata-rata (μs)')
    ax2.set_title('Latency per Kecepatan')
    ax2.grid(True, alpha=0.3, axis='y')
    for bar, lat in zip(bars2, latencies):
        ax2.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                 f'{lat:.0f}', ha='center', va='bottom', fontweight='bold')

    # Plot 3: Throughput vs Waktu (Line Chart)
    ax3 = axes[1, 0]
    for spd in speeds:
        data = speed_groups[spd]
        t = [d['timestamp'] for d in data]
        tp = [d['throughput_bps'] / 1024 for d in data]
        ax3.plot(t, tp, 'o-', markersize=5, label=f'{spd//1000}kHz')
    ax3.set_xlabel('Waktu (detik)')
    ax3.set_ylabel('Throughput (KB/s)')
    ax3.set_title('Throughput vs Waktu')
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    # Plot 4: Speedup dan Efisiensi
    ax4 = axes[1, 1]
    if speedup_data:
        ratios = [s['ratio'] for s in speedup_data]
        efficiencies = [s['efficiency'] for s in speedup_data]
        x = list(range(len(ratios)))

        ax4_twin = ax4.twinx()
        bars_r = ax4.bar([i - 0.2 for i in x], ratios, 0.4, color='steelblue',
                         alpha=0.7, label='Rasio Percepatan')
        bars_e = ax4_twin.bar([i + 0.2 for i in x], efficiencies, 0.4,
                              color='orange', alpha=0.7, label='Efisiensi (%)')

        ax4.set_xlabel('Pengujian #')
        ax4.set_ylabel('Rasio Percepatan (x)', color='steelblue')
        ax4_twin.set_ylabel('Efisiensi (%)', color='orange')
        ax4.set_title('Percepatan 400kHz vs 100kHz')
        ax4.axhline(y=4.0, color='red', linestyle='--', alpha=0.5, label='Ideal (4x)')
        ax4.legend(loc='upper left')
        ax4_twin.legend(loc='upper right')
        ax4.grid(True, alpha=0.3)
    else:
        if len(throughputs) >= 2 and throughputs[0] > 0:
            ratio = throughputs[1] / throughputs[0]
            eff = (ratio / 4.0) * 100
            categories = ['Rasio Percepatan', 'Efisiensi (%)']
            values = [ratio, eff]
            colors_sp = ['steelblue', 'orange']
            ax4.bar(categories, values, color=colors_sp, alpha=0.8)
            ax4.set_title(f'Percepatan: {ratio:.2f}x (Efisiensi: {eff:.1f}%)')
            ax4.grid(True, alpha=0.3)
        else:
            ax4.text(0.5, 0.5, 'Data speedup tidak tersedia', ha='center',
                     va='center', transform=ax4.transAxes)

    plt.tight_layout()
    filepath = os.path.join(os.path.dirname(__file__), filename)
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"[INFO] Grafik disimpan ke {filepath}")


def print_summary(speed_results, speedup_data):
    """Menampilkan ringkasan analisis."""
    print("\n" + "=" * 60)
    print("     RINGKASAN PERBANDINGAN KECEPATAN I2C")
    print("=" * 60)

    if not speed_results:
        print("  Tidak ada data untuk dianalisis.")
        return

    # Kelompokkan berdasarkan kecepatan
    speed_groups = {}
    for r in speed_results:
        spd = r['speed_hz']
        if spd not in speed_groups:
            speed_groups[spd] = []
        speed_groups[spd].append(r)

    for spd in sorted(speed_groups.keys()):
        data = speed_groups[spd]
        avg_tp = sum(r['throughput_bps'] for r in data) / len(data)
        avg_lat = sum(r['avg_latency_us'] for r in data) / len(data)
        total_ok = sum(r['success'] for r in data)
        total_fail = sum(r['fail'] for r in data)

        print(f"\n  Kecepatan: {spd/1000:.0f} kHz")
        print(f"    Pengujian      : {len(data)} kali")
        print(f"    Throughput avg : {avg_tp:.2f} B/s ({avg_tp/1024:.2f} KB/s)")
        print(f"    Latency avg    : {avg_lat:.0f} μs")
        print(f"    Total OK/FAIL  : {total_ok}/{total_fail}")

    if speedup_data:
        avg_ratio = sum(s['ratio'] for s in speedup_data) / len(speedup_data)
        avg_eff = sum(s['efficiency'] for s in speedup_data) / len(speedup_data)
        print(f"\n  Percepatan 400kHz vs 100kHz:")
        print(f"    Rasio rata-rata : {avg_ratio:.2f}x")
        print(f"    Efisiensi       : {avg_eff:.1f}%")
        print(f"    Ideal           : 4.00x (100%)")

    print("=" * 60)


def main():
    """Fungsi utama program."""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 120

    print("=" * 60)
    print("  ESP32 I2C Clock Speed Test - Debug Analysis")
    print(f"  Port: {port} | Durasi: {duration}s")
    print("=" * 60)

    speed_results, speedup_data = parse_serial_data(port, duration)

    if not speed_results:
        print("[WARN] Tidak ada data yang terkumpul.")
        return

    save_to_csv(speed_results, CSV_FILENAME)
    create_plots(speed_results, speedup_data, PLOT_FILENAME)
    print_summary(speed_results, speedup_data)


if __name__ == '__main__':
    main()

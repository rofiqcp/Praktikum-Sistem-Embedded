"""
ESP32_07_I2C_Light_BH1750 - Debug Analysis
Modul 06 - I2C & Sensor

Deskripsi: Parser serial + analisis data sensor cahaya BH1750.
           Menghasilkan grafik lux vs waktu dan distribusi klasifikasi.
           Menyimpan data ke CSV untuk analisis lebih lanjut.

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
CSV_FILENAME = 'bh1750_data.csv'
PLOT_FILENAME = 'bh1750_plot.png'
MAX_SAMPLES = 500

def parse_serial_data(port, duration=60):
    """Membaca dan mem-parsing data serial dari ESP32."""
    data_continuous = []
    data_onetime = []
    klasifikasi_count = defaultdict(int)

    print(f"[INFO] Membuka port serial {port} pada {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(2)  # Tunggu koneksi stabil
        ser.reset_input_buffer()
        print(f"[INFO] Port serial terbuka. Membaca data selama {duration} detik...")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti lebih awal.\n")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        return data_continuous, data_onetime, klasifikasi_count

    start_time = time.time()
    sample_count = 0

    try:
        while (time.time() - start_time) < duration and sample_count < MAX_SAMPLES:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue

                # Tampilkan raw data
                print(f"  RAW: {line}")

                # Parse format: DATA,mode,sample,lux,klasifikasi
                if line.startswith('DATA,'):
                    parts = line.split(',')
                    if len(parts) >= 5:
                        try:
                            mode = parts[1]
                            sample_num = int(parts[2])
                            lux = float(parts[3])
                            klasifikasi = parts[4]
                            timestamp = time.time() - start_time

                            entry = {
                                'timestamp': timestamp,
                                'sample': sample_num,
                                'lux': lux,
                                'klasifikasi': klasifikasi,
                                'mode': mode
                            }

                            if mode == 'CONT':
                                data_continuous.append(entry)
                            elif mode == 'ONETIME':
                                data_onetime.append(entry)

                            klasifikasi_count[klasifikasi] += 1
                            sample_count += 1

                            print(f"  [{mode}] Sampel #{sample_num}: "
                                  f"{lux:.2f} lux - {klasifikasi}")
                        except (ValueError, IndexError) as e:
                            print(f"  [WARN] Gagal parse: {e}")

    except KeyboardInterrupt:
        print("\n[INFO] Pembacaan dihentikan oleh pengguna.")
    finally:
        ser.close()
        print(f"[INFO] Port serial ditutup. Total sampel: {sample_count}")

    return data_continuous, data_onetime, klasifikasi_count


def save_to_csv(data_cont, data_onetime, filename):
    """Menyimpan data ke file CSV."""
    all_data = []
    for d in data_cont:
        all_data.append(d)
    for d in data_onetime:
        all_data.append(d)

    # Urutkan berdasarkan timestamp
    all_data.sort(key=lambda x: x['timestamp'])

    filepath = os.path.join(os.path.dirname(__file__), filename)
    with open(filepath, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile,
                                fieldnames=['timestamp', 'sample', 'lux',
                                            'klasifikasi', 'mode'])
        writer.writeheader()
        writer.writerows(all_data)

    print(f"[INFO] Data disimpan ke {filepath} ({len(all_data)} baris)")
    return filepath


def create_plots(data_cont, data_onetime, klasifikasi_count, filename):
    """Membuat grafik analisis data BH1750."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        import matplotlib.dates as mdates
    except ImportError:
        print("[WARN] matplotlib tidak tersedia. Grafik tidak dibuat.")
        print("       Install dengan: pip install matplotlib")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Analisis Sensor Cahaya BH1750 - I2C ESP32', fontsize=14, fontweight='bold')

    # Plot 1: Lux vs Waktu (Continuous Mode)
    ax1 = axes[0, 0]
    if data_cont:
        times = [d['timestamp'] for d in data_cont]
        lux_vals = [d['lux'] for d in data_cont]
        ax1.plot(times, lux_vals, 'b-o', markersize=2, linewidth=1, label='Continuous')
        ax1.set_xlabel('Waktu (detik)')
        ax1.set_ylabel('Intensitas Cahaya (lux)')
        ax1.set_title('Pembacaan Continuous Mode')
        ax1.grid(True, alpha=0.3)
        ax1.legend()

        # Tambahkan garis threshold klasifikasi
        thresholds = [10, 100, 500, 10000]
        labels_th = ['Gelap', 'Redup', 'Normal', 'Terang']
        colors_th = ['gray', 'orange', 'green', 'yellow']
        for th, lbl, col in zip(thresholds, labels_th, colors_th):
            ax1.axhline(y=th, color=col, linestyle='--', alpha=0.5, label=lbl)
    else:
        ax1.text(0.5, 0.5, 'Tidak ada data continuous', ha='center', va='center',
                 transform=ax1.transAxes, fontsize=12, color='gray')

    # Plot 2: Perbandingan Continuous vs One-time
    ax2 = axes[0, 1]
    if data_cont and data_onetime:
        cont_times = [d['timestamp'] for d in data_cont]
        cont_lux = [d['lux'] for d in data_cont]
        ot_times = [d['timestamp'] for d in data_onetime]
        ot_lux = [d['lux'] for d in data_onetime]
        ax2.plot(cont_times, cont_lux, 'b-', linewidth=1, alpha=0.7, label='Continuous')
        ax2.scatter(ot_times, ot_lux, color='red', s=50, zorder=5, label='One-Time')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Intensitas Cahaya (lux)')
        ax2.set_title('Continuous vs One-Time Mode')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
    else:
        ax2.text(0.5, 0.5, 'Data tidak cukup\nuntuk perbandingan', ha='center',
                 va='center', transform=ax2.transAxes, fontsize=12, color='gray')

    # Plot 3: Distribusi Klasifikasi (Pie Chart)
    ax3 = axes[1, 0]
    if klasifikasi_count:
        labels = list(klasifikasi_count.keys())
        sizes = list(klasifikasi_count.values())
        colors_pie = ['#2c3e50', '#e67e22', '#27ae60', '#f1c40f', '#e74c3c']
        ax3.pie(sizes, labels=labels, colors=colors_pie[:len(labels)],
                autopct='%1.1f%%', startangle=90)
        ax3.set_title('Distribusi Klasifikasi Cahaya')
    else:
        ax3.text(0.5, 0.5, 'Tidak ada data klasifikasi', ha='center', va='center',
                 transform=ax3.transAxes, fontsize=12, color='gray')

    # Plot 4: Histogram Lux
    ax4 = axes[1, 1]
    if data_cont:
        lux_all = [d['lux'] for d in data_cont]
        ax4.hist(lux_all, bins=30, color='steelblue', edgecolor='white', alpha=0.8)
        ax4.set_xlabel('Intensitas Cahaya (lux)')
        ax4.set_ylabel('Frekuensi')
        ax4.set_title('Distribusi Nilai Lux')
        ax4.grid(True, alpha=0.3)

        # Statistik
        if lux_all:
            avg_lux = sum(lux_all) / len(lux_all)
            min_lux = min(lux_all)
            max_lux = max(lux_all)
            ax4.axvline(x=avg_lux, color='red', linestyle='--', label=f'Rata-rata: {avg_lux:.1f}')
            ax4.legend()
    else:
        ax4.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax4.transAxes, fontsize=12, color='gray')

    plt.tight_layout()
    filepath = os.path.join(os.path.dirname(__file__), filename)
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"[INFO] Grafik disimpan ke {filepath}")


def print_summary(data_cont, data_onetime, klasifikasi_count):
    """Menampilkan ringkasan analisis."""
    print("\n" + "=" * 60)
    print("          RINGKASAN ANALISIS BH1750")
    print("=" * 60)

    if data_cont:
        lux_vals = [d['lux'] for d in data_cont]
        print(f"\n  Mode Continuous:")
        print(f"    Jumlah sampel  : {len(data_cont)}")
        print(f"    Lux minimum    : {min(lux_vals):.2f}")
        print(f"    Lux maksimum   : {max(lux_vals):.2f}")
        print(f"    Lux rata-rata  : {sum(lux_vals) / len(lux_vals):.2f}")

    if data_onetime:
        lux_ot = [d['lux'] for d in data_onetime]
        print(f"\n  Mode One-Time:")
        print(f"    Jumlah sampel  : {len(data_onetime)}")
        print(f"    Lux rata-rata  : {sum(lux_ot) / len(lux_ot):.2f}")

    if klasifikasi_count:
        print(f"\n  Distribusi Klasifikasi:")
        total = sum(klasifikasi_count.values())
        for klas, count in sorted(klasifikasi_count.items()):
            pct = (count / total) * 100
            print(f"    {klas:25s}: {count:4d} ({pct:.1f}%)")

    print("=" * 60)


def main():
    """Fungsi utama program."""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    print("=" * 60)
    print("  ESP32 BH1750 Light Sensor - Debug Analysis")
    print(f"  Port: {port} | Durasi: {duration}s")
    print("=" * 60)

    # Baca data serial
    data_cont, data_onetime, klasifikasi_count = parse_serial_data(port, duration)

    if not data_cont and not data_onetime:
        print("[WARN] Tidak ada data yang terkumpul.")
        print("       Pastikan ESP32 terhubung dan mengirim data.")
        return

    # Simpan ke CSV
    save_to_csv(data_cont, data_onetime, CSV_FILENAME)

    # Buat grafik
    create_plots(data_cont, data_onetime, klasifikasi_count, PLOT_FILENAME)

    # Tampilkan ringkasan
    print_summary(data_cont, data_onetime, klasifikasi_count)


if __name__ == '__main__':
    main()

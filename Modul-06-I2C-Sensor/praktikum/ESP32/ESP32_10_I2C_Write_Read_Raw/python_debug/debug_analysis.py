"""
ESP32_10_I2C_Write_Read_Raw - Debug Analysis
Modul 06 - I2C & Sensor

Deskripsi: Parser serial + analisis demonstrasi protokol I2C raw.
           Memvisualisasikan hasil scan bus, transaksi read/write,
           dan waktu eksekusi tiap transaksi.

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
CSV_FILENAME = 'i2c_raw_data.csv'
PLOT_FILENAME = 'i2c_raw_plot.png'
MAX_SAMPLES = 500

def parse_serial_data(port, duration=60):
    """Membaca dan mem-parsing data serial dari ESP32."""
    transactions = []
    scan_results = []
    write_results = []
    multi_reads = []

    print(f"[INFO] Membuka port serial {port} pada {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(2)
        ser.reset_input_buffer()
        print(f"[INFO] Port serial terbuka. Membaca data selama {duration} detik...")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti lebih awal.\n")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        return transactions, scan_results, write_results, multi_reads

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

                # Parse format: DATA,addr,reg,status,time_us,data
                if line.startswith('DATA,'):
                    parts = line.split(',')
                    if len(parts) >= 6:
                        try:
                            entry = {
                                'timestamp': timestamp,
                                'addr': parts[1],
                                'reg': parts[2],
                                'status': parts[3],
                                'time_us': int(parts[4]) if parts[4] != 'N/A' else 0,
                                'data': parts[5]
                            }
                            transactions.append(entry)
                            sample_count += 1

                            status_icon = "✓" if parts[3] == "OK" else "✗"
                            print(f"  [{status_icon}] Addr={parts[1]} Reg={parts[2]} "
                                  f"Status={parts[3]} Time={parts[4]}us Data={parts[5]}")
                        except (ValueError, IndexError) as e:
                            print(f"  [WARN] Gagal parse DATA: {e}")

                # Parse format: SCAN,found,N,time_ms,M
                elif line.startswith('SCAN,'):
                    parts = line.split(',')
                    if len(parts) >= 5:
                        scan_results.append({
                            'timestamp': timestamp,
                            'found': int(parts[2]),
                            'time_ms': int(parts[4])
                        })
                        print(f"  [SCAN] Ditemukan {parts[2]} perangkat dalam {parts[4]}ms")

                # Parse format: WRITE,addr,reg,status
                elif line.startswith('WRITE,'):
                    parts = line.split(',')
                    if len(parts) >= 4:
                        write_results.append({
                            'timestamp': timestamp,
                            'addr': parts[1],
                            'reg': parts[2],
                            'status': parts[3]
                        })

                # Parse format: MULTI_READ,addr,reg,count,status,data...
                elif line.startswith('MULTI_READ,'):
                    parts = line.split(',')
                    if len(parts) >= 5:
                        multi_reads.append({
                            'timestamp': timestamp,
                            'addr': parts[1],
                            'reg': parts[2],
                            'count': int(parts[3]),
                            'status': parts[4],
                            'data': ','.join(parts[5:]) if len(parts) > 5 else ''
                        })

    except KeyboardInterrupt:
        print("\n[INFO] Pembacaan dihentikan oleh pengguna.")
    finally:
        ser.close()
        print(f"[INFO] Port serial ditutup. Total sampel: {sample_count}")

    return transactions, scan_results, write_results, multi_reads


def save_to_csv(transactions, filename):
    """Menyimpan data transaksi ke CSV."""
    filepath = os.path.join(os.path.dirname(__file__), filename)
    with open(filepath, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile,
                                fieldnames=['timestamp', 'addr', 'reg', 'status',
                                            'time_us', 'data'])
        writer.writeheader()
        writer.writerows(transactions)

    print(f"[INFO] Data disimpan ke {filepath} ({len(transactions)} baris)")
    return filepath


def create_plots(transactions, scan_results, filename):
    """Membuat grafik analisis protokol I2C raw."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        print("[WARN] matplotlib tidak tersedia. Grafik tidak dibuat.")
        print("       Install dengan: pip install matplotlib")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Analisis Protokol I2C Raw - ESP32', fontsize=14, fontweight='bold')

    # Plot 1: Waktu Eksekusi per Transaksi
    ax1 = axes[0, 0]
    if transactions:
        times_exec = [t['time_us'] for t in transactions if t['time_us'] > 0]
        indices = list(range(len(times_exec)))
        colors = ['green' if t['status'] == 'OK' else 'red'
                  for t in transactions if t['time_us'] > 0]
        ax1.bar(indices, times_exec, color=colors, alpha=0.7)
        ax1.set_xlabel('Transaksi #')
        ax1.set_ylabel('Waktu Eksekusi (μs)')
        ax1.set_title('Waktu Eksekusi per Transaksi I2C')
        ax1.grid(True, alpha=0.3)
        if times_exec:
            avg_t = sum(times_exec) / len(times_exec)
            ax1.axhline(y=avg_t, color='blue', linestyle='--',
                        label=f'Rata-rata: {avg_t:.0f}μs')
            ax1.legend()
    else:
        ax1.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax1.transAxes)

    # Plot 2: Status Transaksi (Pie Chart)
    ax2 = axes[0, 1]
    if transactions:
        status_count = defaultdict(int)
        for t in transactions:
            status_count[t['status']] += 1
        labels = list(status_count.keys())
        sizes = list(status_count.values())
        colors_pie = ['#27ae60' if l == 'OK' else '#e74c3c' for l in labels]
        ax2.pie(sizes, labels=labels, colors=colors_pie, autopct='%1.1f%%',
                startangle=90)
        ax2.set_title('Distribusi Status Transaksi')
    else:
        ax2.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax2.transAxes)

    # Plot 3: Distribusi Alamat yang Diakses
    ax3 = axes[1, 0]
    if transactions:
        addr_count = defaultdict(int)
        for t in transactions:
            addr_count[t['addr']] += 1
        addrs = list(addr_count.keys())
        counts = list(addr_count.values())
        ax3.bar(addrs, counts, color='steelblue', alpha=0.8)
        ax3.set_xlabel('Alamat I2C')
        ax3.set_ylabel('Jumlah Transaksi')
        ax3.set_title('Distribusi Alamat I2C')
        ax3.grid(True, alpha=0.3)
    else:
        ax3.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax3.transAxes)

    # Plot 4: Histogram Waktu Eksekusi
    ax4 = axes[1, 1]
    if transactions:
        times_exec = [t['time_us'] for t in transactions if t['time_us'] > 0]
        if times_exec:
            ax4.hist(times_exec, bins=20, color='steelblue', edgecolor='white', alpha=0.8)
            ax4.set_xlabel('Waktu Eksekusi (μs)')
            ax4.set_ylabel('Frekuensi')
            ax4.set_title('Distribusi Waktu Eksekusi')
            ax4.grid(True, alpha=0.3)
    else:
        ax4.text(0.5, 0.5, 'Tidak ada data', ha='center', va='center',
                 transform=ax4.transAxes)

    plt.tight_layout()
    filepath = os.path.join(os.path.dirname(__file__), filename)
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"[INFO] Grafik disimpan ke {filepath}")


def print_summary(transactions, scan_results, write_results, multi_reads):
    """Menampilkan ringkasan analisis."""
    print("\n" + "=" * 60)
    print("      RINGKASAN ANALISIS PROTOKOL I2C RAW")
    print("=" * 60)

    print(f"\n  Total transaksi read   : {len(transactions)}")
    print(f"  Total transaksi write  : {len(write_results)}")
    print(f"  Total multi-byte read  : {len(multi_reads)}")
    print(f"  Total scan bus         : {len(scan_results)}")

    if transactions:
        ok_count = sum(1 for t in transactions if t['status'] == 'OK')
        fail_count = len(transactions) - ok_count
        times_exec = [t['time_us'] for t in transactions if t['time_us'] > 0]

        print(f"\n  Statistik Transaksi Read:")
        print(f"    Berhasil     : {ok_count}")
        print(f"    Gagal        : {fail_count}")
        if times_exec:
            print(f"    Waktu min    : {min(times_exec)} μs")
            print(f"    Waktu max    : {max(times_exec)} μs")
            print(f"    Waktu avg    : {sum(times_exec)/len(times_exec):.0f} μs")

    if scan_results:
        print(f"\n  Hasil Scan Bus:")
        for s in scan_results:
            print(f"    Perangkat: {s['found']}, Waktu: {s['time_ms']}ms")

    print("=" * 60)


def main():
    """Fungsi utama program."""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    print("=" * 60)
    print("  ESP32 I2C Write/Read Raw - Debug Analysis")
    print(f"  Port: {port} | Durasi: {duration}s")
    print("=" * 60)

    transactions, scan_results, write_results, multi_reads = \
        parse_serial_data(port, duration)

    if not transactions and not scan_results:
        print("[WARN] Tidak ada data yang terkumpul.")
        return

    save_to_csv(transactions, CSV_FILENAME)
    create_plots(transactions, scan_results, PLOT_FILENAME)
    print_summary(transactions, scan_results, write_results, multi_reads)


if __name__ == '__main__':
    main()

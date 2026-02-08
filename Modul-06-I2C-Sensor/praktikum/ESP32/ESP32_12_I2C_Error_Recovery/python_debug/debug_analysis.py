"""
ESP32_12_I2C_Error_Recovery - Debug Analysis
Modul 06 - I2C & Sensor

Deskripsi: Parser serial + analisis error dan recovery I2C.
           Memvisualisasikan statistik error, recovery success rate,
           dan timeline event error/recovery.

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
CSV_FILENAME = 'i2c_error_recovery_data.csv'
PLOT_FILENAME = 'i2c_error_recovery_plot.png'
MAX_SAMPLES = 1000

def parse_serial_data(port, duration=120):
    """Membaca dan mem-parsing data serial error recovery I2C."""
    data_events = []
    error_events = []
    recovery_events = []
    stats_snapshots = []

    print(f"[INFO] Membuka port serial {port} pada {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(2)
        ser.reset_input_buffer()
        print(f"[INFO] Port serial terbuka. Membaca data selama {duration} detik...")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti lebih awal.\n")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        return data_events, error_events, recovery_events, stats_snapshots

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

                # Parse: DATA,type,addr,reg,status,data
                if line.startswith('DATA,'):
                    parts = line.split(',')
                    if len(parts) >= 4:
                        entry = {
                            'timestamp': timestamp,
                            'type': parts[1],
                            'detail': ','.join(parts[2:])
                        }
                        data_events.append(entry)
                        sample_count += 1

                        if parts[1] == 'BUS_CHECK':
                            status = parts[2] if len(parts) > 2 else 'UNKNOWN'
                            icon = "⚠" if status == 'BUSY' else "✓"
                            print(f"  [{icon}] Bus: {status}")
                        elif len(parts) >= 5:
                            print(f"  [DATA] {parts[1]}: addr={parts[2]} "
                                  f"status={parts[4]}")

                # Parse: ERROR,type,addr,reg,attempt
                elif line.startswith('ERROR,'):
                    parts = line.split(',')
                    if len(parts) >= 5:
                        entry = {
                            'timestamp': timestamp,
                            'error_type': parts[1],
                            'addr': parts[2],
                            'reg': parts[3],
                            'attempt': int(parts[4])
                        }
                        error_events.append(entry)
                        sample_count += 1
                        print(f"  [ERROR] {parts[1]} addr={parts[2]} "
                              f"percobaan ke-{parts[4]}")

                # Parse: RECOVERY,status,attempts,successes
                elif line.startswith('RECOVERY,'):
                    parts = line.split(',')
                    if len(parts) >= 4:
                        entry = {
                            'timestamp': timestamp,
                            'status': parts[1],
                            'attempts': int(parts[2]),
                            'successes': int(parts[3])
                        }
                        recovery_events.append(entry)
                        sample_count += 1
                        icon = "✓" if parts[1] == 'SUCCESS' else "✗"
                        print(f"  [{icon}] Recovery {parts[1]}: "
                              f"attempts={parts[2]} success={parts[3]}")

                # Parse: STATS,total,success,nack,timeout,busy,other,
                #        rec_attempts,rec_success,retry_success,rate
                elif line.startswith('STATS,'):
                    parts = line.split(',')
                    if len(parts) >= 11:
                        try:
                            entry = {
                                'timestamp': timestamp,
                                'total': int(parts[1]),
                                'success': int(parts[2]),
                                'nack': int(parts[3]),
                                'timeout': int(parts[4]),
                                'busy': int(parts[5]),
                                'other': int(parts[6]),
                                'rec_attempts': int(parts[7]),
                                'rec_success': int(parts[8]),
                                'retry_success': int(parts[9]),
                                'success_rate': float(parts[10])
                            }
                            stats_snapshots.append(entry)
                            print(f"  [STATS] Rate={entry['success_rate']:.1f}% "
                                  f"Total={entry['total']} OK={entry['success']} "
                                  f"NACK={entry['nack']}")
                        except (ValueError, IndexError) as e:
                            print(f"  [WARN] Gagal parse STATS: {e}")

    except KeyboardInterrupt:
        print("\n[INFO] Pembacaan dihentikan oleh pengguna.")
    finally:
        ser.close()
        print(f"[INFO] Port serial ditutup. Total event: {sample_count}")

    return data_events, error_events, recovery_events, stats_snapshots


def save_to_csv(data_events, error_events, recovery_events, stats_snapshots, filename):
    """Menyimpan data ke file CSV."""
    base_path = os.path.dirname(__file__)

    # Simpan event data
    filepath = os.path.join(base_path, filename)
    with open(filepath, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile,
                                fieldnames=['timestamp', 'type', 'detail'])
        writer.writeheader()
        writer.writerows(data_events)
    print(f"[INFO] Data event disimpan ke {filepath} ({len(data_events)} baris)")

    # Simpan error events
    error_file = os.path.join(base_path, filename.replace('.csv', '_errors.csv'))
    if error_events:
        with open(error_file, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile,
                                    fieldnames=['timestamp', 'error_type', 'addr',
                                                'reg', 'attempt'])
            writer.writeheader()
            writer.writerows(error_events)
        print(f"[INFO] Error log disimpan ke {error_file} ({len(error_events)} baris)")

    # Simpan recovery events
    rec_file = os.path.join(base_path, filename.replace('.csv', '_recovery.csv'))
    if recovery_events:
        with open(rec_file, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile,
                                    fieldnames=['timestamp', 'status', 'attempts',
                                                'successes'])
            writer.writeheader()
            writer.writerows(recovery_events)
        print(f"[INFO] Recovery log disimpan ke {rec_file} ({len(recovery_events)} baris)")

    # Simpan statistik
    stats_file = os.path.join(base_path, filename.replace('.csv', '_stats.csv'))
    if stats_snapshots:
        with open(stats_file, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile,
                                    fieldnames=['timestamp', 'total', 'success',
                                                'nack', 'timeout', 'busy', 'other',
                                                'rec_attempts', 'rec_success',
                                                'retry_success', 'success_rate'])
            writer.writeheader()
            writer.writerows(stats_snapshots)
        print(f"[INFO] Statistik disimpan ke {stats_file} ({len(stats_snapshots)} baris)")

    return filepath


def create_plots(data_events, error_events, recovery_events, stats_snapshots, filename):
    """Membuat grafik analisis error dan recovery I2C."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        print("[WARN] matplotlib tidak tersedia. Grafik tidak dibuat.")
        print("       Install dengan: pip install matplotlib")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Analisis Error & Recovery I2C - ESP32',
                 fontsize=14, fontweight='bold')

    # Plot 1: Timeline Event (Error vs Success)
    ax1 = axes[0, 0]
    if data_events:
        # Pisahkan berdasarkan tipe
        normal_t = [d['timestamp'] for d in data_events if d['type'] == 'NORMAL']
        nack_t = [d['timestamp'] for d in data_events if d['type'] == 'NACK_TEST']
        bus_t = [d['timestamp'] for d in data_events if d['type'] == 'BUS_CHECK']

        if normal_t:
            ax1.scatter(normal_t, [1] * len(normal_t), c='green', s=30,
                        label='Normal', alpha=0.7, marker='o')
        if nack_t:
            ax1.scatter(nack_t, [2] * len(nack_t), c='red', s=30,
                        label='NACK Test', alpha=0.7, marker='x')
        if bus_t:
            ax1.scatter(bus_t, [3] * len(bus_t), c='orange', s=30,
                        label='Bus Check', alpha=0.7, marker='^')

        # Tandai recovery events
        if recovery_events:
            rec_ok = [r['timestamp'] for r in recovery_events if r['status'] == 'SUCCESS']
            rec_fail = [r['timestamp'] for r in recovery_events if r['status'] != 'SUCCESS']
            if rec_ok:
                ax1.scatter(rec_ok, [4] * len(rec_ok), c='blue', s=80,
                            label='Recovery OK', alpha=0.8, marker='*')
            if rec_fail:
                ax1.scatter(rec_fail, [4] * len(rec_fail), c='purple', s=80,
                            label='Recovery FAIL', alpha=0.8, marker='*')

        ax1.set_xlabel('Waktu (detik)')
        ax1.set_yticks([1, 2, 3, 4])
        ax1.set_yticklabels(['Normal', 'NACK', 'Bus Check', 'Recovery'])
        ax1.set_title('Timeline Event I2C')
        ax1.legend(loc='upper right', fontsize=8)
        ax1.grid(True, alpha=0.3)
    else:
        ax1.text(0.5, 0.5, 'Tidak ada data event', ha='center', va='center',
                 transform=ax1.transAxes)

    # Plot 2: Distribusi Error (Pie Chart)
    ax2 = axes[0, 1]
    if error_events:
        error_types = defaultdict(int)
        for e in error_events:
            error_types[e['error_type']] += 1

        labels = list(error_types.keys())
        sizes = list(error_types.values())
        colors_pie = ['#e74c3c', '#f39c12', '#9b59b6', '#34495e']
        ax2.pie(sizes, labels=labels, colors=colors_pie[:len(labels)],
                autopct='%1.1f%%', startangle=90)
        ax2.set_title('Distribusi Jenis Error')
    elif stats_snapshots:
        # Gunakan statistik terakhir
        last = stats_snapshots[-1]
        labels = []
        sizes = []
        if last['nack'] > 0:
            labels.append('NACK')
            sizes.append(last['nack'])
        if last['timeout'] > 0:
            labels.append('Timeout')
            sizes.append(last['timeout'])
        if last['busy'] > 0:
            labels.append('Bus Busy')
            sizes.append(last['busy'])
        if last['other'] > 0:
            labels.append('Lainnya')
            sizes.append(last['other'])
        if last['success'] > 0:
            labels.append('Berhasil')
            sizes.append(last['success'])

        if sizes:
            colors_pie = ['#e74c3c', '#f39c12', '#9b59b6', '#34495e', '#27ae60']
            ax2.pie(sizes, labels=labels, colors=colors_pie[:len(labels)],
                    autopct='%1.1f%%', startangle=90)
            ax2.set_title('Distribusi Hasil Transaksi')
    else:
        ax2.text(0.5, 0.5, 'Tidak ada data error', ha='center', va='center',
                 transform=ax2.transAxes)

    # Plot 3: Success Rate vs Waktu
    ax3 = axes[1, 0]
    if stats_snapshots:
        times_s = [s['timestamp'] for s in stats_snapshots]
        rates = [s['success_rate'] for s in stats_snapshots]
        ax3.plot(times_s, rates, 'g-o', markersize=5, linewidth=2, label='Success Rate')
        ax3.axhline(y=100, color='green', linestyle='--', alpha=0.3, label='Ideal (100%)')
        ax3.axhline(y=50, color='red', linestyle='--', alpha=0.3, label='Kritis (50%)')
        ax3.set_xlabel('Waktu (detik)')
        ax3.set_ylabel('Success Rate (%)')
        ax3.set_title('Tingkat Keberhasilan vs Waktu')
        ax3.set_ylim(0, 110)
        ax3.legend()
        ax3.grid(True, alpha=0.3)
    else:
        ax3.text(0.5, 0.5, 'Tidak ada data statistik', ha='center', va='center',
                 transform=ax3.transAxes)

    # Plot 4: Recovery Statistics
    ax4 = axes[1, 1]
    if stats_snapshots:
        last = stats_snapshots[-1]
        categories = ['Total TX', 'Berhasil', 'NACK', 'Timeout',
                      'Recovery\nAttempt', 'Recovery\nOK', 'Retry\nOK']
        values = [last['total'], last['success'], last['nack'], last['timeout'],
                  last['rec_attempts'], last['rec_success'], last['retry_success']]
        colors_bar = ['#3498db', '#27ae60', '#e74c3c', '#f39c12',
                      '#9b59b6', '#2ecc71', '#1abc9c']
        bars = ax4.bar(categories, values, color=colors_bar, alpha=0.8)
        ax4.set_ylabel('Jumlah')
        ax4.set_title('Ringkasan Statistik Error & Recovery')
        ax4.grid(True, alpha=0.3, axis='y')

        for bar, val in zip(bars, values):
            if val > 0:
                ax4.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                         str(val), ha='center', va='bottom', fontweight='bold', fontsize=9)
    elif recovery_events:
        rec_ok = sum(1 for r in recovery_events if r['status'] == 'SUCCESS')
        rec_fail = len(recovery_events) - rec_ok
        ax4.bar(['Recovery OK', 'Recovery FAIL'], [rec_ok, rec_fail],
                color=['#27ae60', '#e74c3c'], alpha=0.8)
        ax4.set_title('Hasil Recovery')
        ax4.grid(True, alpha=0.3, axis='y')
    else:
        ax4.text(0.5, 0.5, 'Tidak ada data recovery', ha='center', va='center',
                 transform=ax4.transAxes)

    plt.tight_layout()
    filepath = os.path.join(os.path.dirname(__file__), filename)
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"[INFO] Grafik disimpan ke {filepath}")


def print_summary(data_events, error_events, recovery_events, stats_snapshots):
    """Menampilkan ringkasan analisis."""
    print("\n" + "=" * 60)
    print("     RINGKASAN ANALISIS ERROR & RECOVERY I2C")
    print("=" * 60)

    print(f"\n  Total event data    : {len(data_events)}")
    print(f"  Total event error   : {len(error_events)}")
    print(f"  Total event recovery: {len(recovery_events)}")

    if error_events:
        error_types = defaultdict(int)
        for e in error_events:
            error_types[e['error_type']] += 1

        print(f"\n  Distribusi Error:")
        for etype, count in sorted(error_types.items()):
            print(f"    {etype:15s}: {count}")

    if recovery_events:
        rec_ok = sum(1 for r in recovery_events if r['status'] == 'SUCCESS')
        rec_fail = len(recovery_events) - rec_ok
        rate = (rec_ok / len(recovery_events) * 100) if recovery_events else 0

        print(f"\n  Recovery:")
        print(f"    Total percobaan : {len(recovery_events)}")
        print(f"    Berhasil        : {rec_ok}")
        print(f"    Gagal           : {rec_fail}")
        print(f"    Success rate    : {rate:.1f}%")

    if stats_snapshots:
        last = stats_snapshots[-1]
        print(f"\n  Statistik Terakhir:")
        print(f"    Total transaksi  : {last['total']}")
        print(f"    Berhasil         : {last['success']}")
        print(f"    NACK errors      : {last['nack']}")
        print(f"    Timeout errors   : {last['timeout']}")
        print(f"    Bus busy errors  : {last['busy']}")
        print(f"    Success rate     : {last['success_rate']:.1f}%")
        print(f"    Recovery attempts: {last['rec_attempts']}")
        print(f"    Recovery success : {last['rec_success']}")

    print("=" * 60)


def main():
    """Fungsi utama program."""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 120

    print("=" * 60)
    print("  ESP32 I2C Error Recovery - Debug Analysis")
    print(f"  Port: {port} | Durasi: {duration}s")
    print("=" * 60)

    data_events, error_events, recovery_events, stats_snapshots = \
        parse_serial_data(port, duration)

    if not data_events and not error_events and not stats_snapshots:
        print("[WARN] Tidak ada data yang terkumpul.")
        return

    save_to_csv(data_events, error_events, recovery_events,
                stats_snapshots, CSV_FILENAME)
    create_plots(data_events, error_events, recovery_events,
                 stats_snapshots, PLOT_FILENAME)
    print_summary(data_events, error_events, recovery_events, stats_snapshots)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
============================================================================
Debug Script: STM32_12_Task_Cooperative
============================================================================
Parser & Visualisasi data dari STM32_12_Task_Cooperative

Fitur:
  - Parse output [DATA]WORKER dan [DATA]PHASE dari serial UART
  - Grafik perbandingan eksekusi per worker per fase
  - Grafik fairness index per fase
  - Grafik distribusi persentase CPU per worker
  - Analisis starvation effect pada fase CPU hog
  - Mode demo dengan data simulasi

Penggunaan:
  python debug_cooperative.py --port COM3
  python debug_cooperative.py --file output.log
  python debug_cooperative.py --demo

Dependensi:
  pip install pyserial matplotlib
============================================================================
"""

import argparse
import sys
import re
import time
from collections import defaultdict
from datetime import datetime

# ==========================================================================
# KONFIGURASI
# ==========================================================================

# Regex patterns untuk parsing data tag
PATTERN_WORKER = re.compile(
    r'\[DATA\]WORKER\s+id=(\d+),count=(\d+),phase=(\d+)'
)
PATTERN_PHASE = re.compile(
    r'\[DATA\]PHASE\s+phase=(\d+),total_count=(\d+),fairness=([\d.]+)'
)

# Nama fase
PHASE_NAMES = {
    1: 'Fase 1: Preemptive\n(vTaskDelay)',
    2: 'Fase 2: Cooperative\n(taskYIELD)',
    3: 'Fase 3: CPU Hog\n(Starvation)',
}

PHASE_SHORT_NAMES = {
    1: 'Preemptive',
    2: 'Cooperative',
    3: 'CPU Hog',
}

# Warna per worker
WORKER_COLORS = ['#3498db', '#e74c3c', '#2ecc71']
WORKER_NAMES = ['Worker-0', 'Worker-1', 'Worker-2']

# Warna per fase
PHASE_COLORS = {
    1: '#27ae60',
    2: '#f39c12',
    3: '#e74c3c',
}

NUM_WORKERS = 3
NUM_PHASES = 3

# ==========================================================================
# KELAS DATA STORE
# ==========================================================================

class CooperativeDataStore:
    """Penyimpanan data cooperative scheduling yang di-parse."""

    def __init__(self):
        # Counter terbaru per worker per fase
        self.worker_counts = defaultdict(lambda: defaultdict(int))
        # History per worker {(phase, worker_id): [counts...]}
        self.worker_history = defaultdict(list)
        # Data ringkasan fase {phase: {total_count, fairness}}
        self.phase_summary = {}
        # Timestamp
        self.timestamps = []
        self.start_time = time.time()

    def update_worker(self, worker_id, count, phase):
        """Perbarui counter worker."""
        wid = int(worker_id)
        cnt = int(count)
        ph = int(phase)

        self.worker_counts[ph][wid] = cnt
        self.worker_history[(ph, wid)].append({
            'timestamp': time.time() - self.start_time,
            'count': cnt,
        })

    def update_phase(self, phase, total_count, fairness):
        """Perbarui ringkasan fase."""
        ph = int(phase)
        self.phase_summary[ph] = {
            'total_count': int(total_count),
            'fairness': float(fairness),
        }

    def get_phase_worker_counts(self, phase):
        """Dapatkan counter per worker untuk fase tertentu."""
        counts = []
        for wid in range(NUM_WORKERS):
            counts.append(self.worker_counts.get(phase, {}).get(wid, 0))
        return counts

    def get_all_fairness(self):
        """Dapatkan fairness index semua fase."""
        result = {}
        for ph in range(1, NUM_PHASES + 1):
            if ph in self.phase_summary:
                result[ph] = self.phase_summary[ph]['fairness']
            else:
                result[ph] = 0.0
        return result


# ==========================================================================
# FUNGSI PARSING
# ==========================================================================

def parse_line(line, store):
    """Parse satu baris output serial."""
    line = line.strip()

    # Parse [DATA]WORKER
    match = PATTERN_WORKER.search(line)
    if match:
        worker_id, count, phase = match.groups()
        store.update_worker(worker_id, count, phase)
        return 'worker'

    # Parse [DATA]PHASE
    match = PATTERN_PHASE.search(line)
    if match:
        phase, total_count, fairness = match.groups()
        store.update_phase(phase, total_count, fairness)
        return 'phase'

    return None


# ==========================================================================
# FUNGSI VISUALISASI
# ==========================================================================

def create_visualization(store):
    """Buat visualisasi lengkap dari data cooperative scheduling."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        import matplotlib.patches as mpatches
        from matplotlib.gridspec import GridSpec
        import numpy as np
    except ImportError:
        print("[ERROR] matplotlib tidak tersedia. Install: pip install matplotlib")
        return

    fig = plt.figure(figsize=(18, 16))
    fig.suptitle('STM32_12 — Cooperative Scheduling Analysis Dashboard',
                 fontsize=16, fontweight='bold', y=0.98)
    gs = GridSpec(3, 3, figure=fig, hspace=0.40, wspace=0.30)

    # -- 1. Eksekusi per Worker per Fase (atas kiri, span 2 kolom) --
    ax_bar = fig.add_subplot(gs[0, :2])
    ax_bar.set_title('Jumlah Eksekusi per Worker per Fase', fontweight='bold', fontsize=12)

    phases = list(range(1, NUM_PHASES + 1))
    x = np.arange(len(phases))
    width = 0.25

    has_data = False
    for wid in range(NUM_WORKERS):
        counts = [store.worker_counts.get(ph, {}).get(wid, 0) for ph in phases]
        if any(c > 0 for c in counts):
            has_data = True
        bars = ax_bar.bar(x + wid * width, counts, width,
                         label=WORKER_NAMES[wid], color=WORKER_COLORS[wid],
                         edgecolor='white', linewidth=0.5)
        # Tampilkan angka di atas bar
        for bar, val in zip(bars, counts):
            if val > 0:
                ax_bar.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                           str(val), ha='center', va='bottom', fontsize=8, fontweight='bold')

    if has_data:
        ax_bar.set_xticks(x + width)
        ax_bar.set_xticklabels([PHASE_SHORT_NAMES.get(p, f'Fase {p}') for p in phases])
        ax_bar.set_ylabel('Jumlah Eksekusi')
        ax_bar.legend(fontsize=9)
        ax_bar.grid(True, axis='y', alpha=0.3)
    else:
        ax_bar.text(0.5, 0.5, 'Belum ada data eksekusi',
                   ha='center', va='center', transform=ax_bar.transAxes, color='gray')

    # -- 2. Fairness Index per Fase (atas kanan) --
    ax_fair = fig.add_subplot(gs[0, 2])
    ax_fair.set_title('Fairness Index per Fase', fontweight='bold', fontsize=12)

    fairness_data = store.get_all_fairness()
    if any(v > 0 for v in fairness_data.values()):
        phase_labels = [PHASE_SHORT_NAMES.get(p, f'F{p}') for p in phases]
        fair_values = [fairness_data.get(p, 0.0) for p in phases]
        fair_colors = []
        for f in fair_values:
            if f > 0.95:
                fair_colors.append('#27ae60')
            elif f > 0.80:
                fair_colors.append('#f39c12')
            elif f > 0.50:
                fair_colors.append('#e67e22')
            else:
                fair_colors.append('#e74c3c')

        bars = ax_fair.bar(phase_labels, fair_values, color=fair_colors,
                          edgecolor='white', linewidth=0.5)

        # Garis referensi
        ax_fair.axhline(y=1.0, color='green', linestyle='--', alpha=0.5, label='Sempurna (1.0)')
        ax_fair.axhline(y=1.0/NUM_WORKERS, color='red', linestyle='--', alpha=0.5,
                       label=f'Terburuk ({1.0/NUM_WORKERS:.2f})')

        for bar, val in zip(bars, fair_values):
            ax_fair.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01,
                        f'{val:.3f}', ha='center', va='bottom', fontsize=10, fontweight='bold')

        ax_fair.set_ylabel('Jain\'s Fairness Index')
        ax_fair.set_ylim(0, 1.15)
        ax_fair.legend(fontsize=8, loc='lower right')
        ax_fair.grid(True, axis='y', alpha=0.3)
    else:
        ax_fair.text(0.5, 0.5, 'Belum ada data fairness',
                   ha='center', va='center', transform=ax_fair.transAxes, color='gray')

    # -- 3-5. Distribusi CPU per fase (tengah, 3 pie chart) --
    for idx, phase in enumerate(phases):
        ax_pie = fig.add_subplot(gs[1, idx])
        ax_pie.set_title(PHASE_NAMES.get(phase, f'Fase {phase}'),
                        fontweight='bold', fontsize=11)

        counts = store.get_phase_worker_counts(phase)
        total = sum(counts)

        if total > 0:
            labels = [f'{WORKER_NAMES[i]}\n({counts[i]:,})' for i in range(NUM_WORKERS)]
            colors = WORKER_COLORS[:NUM_WORKERS]

            # Explode worker dominan
            max_idx = counts.index(max(counts))
            explode = [0.05 if i == max_idx else 0 for i in range(NUM_WORKERS)]

            wedges, texts, autotexts = ax_pie.pie(
                counts, labels=labels, colors=colors,
                autopct='%1.1f%%', startangle=90, explode=explode,
                textprops={'fontsize': 9}
            )
            for t in autotexts:
                t.set_fontweight('bold')
                t.set_fontsize(10)

            # Tampilkan fairness di bawah pie
            fairness = fairness_data.get(phase, 0.0)
            quality = ('MERATA' if fairness > 0.95 else
                       'CUKUP' if fairness > 0.80 else
                       'KURANG' if fairness > 0.50 else 'TIDAK MERATA')
            ax_pie.text(0.5, -0.1, f'Fairness: {fairness:.3f} ({quality})',
                       transform=ax_pie.transAxes, ha='center', fontsize=9,
                       fontweight='bold',
                       color='green' if fairness > 0.80 else 'red')
        else:
            ax_pie.text(0.5, 0.5, 'Belum ada data',
                       ha='center', va='center', color='gray', fontsize=11)

    # -- 6. Total Eksekusi Gabungan per Fase (bawah kiri) --
    ax_total = fig.add_subplot(gs[2, 0])
    ax_total.set_title('Total Eksekusi per Fase', fontweight='bold', fontsize=12)

    if store.phase_summary:
        phase_labels = [PHASE_SHORT_NAMES.get(p, f'F{p}') for p in phases]
        totals = [store.phase_summary.get(p, {}).get('total_count', 0) for p in phases]
        colors_total = [PHASE_COLORS.get(p, '#bdc3c7') for p in phases]

        bars = ax_total.bar(phase_labels, totals, color=colors_total,
                          edgecolor='white', linewidth=0.5)
        for bar, val in zip(bars, totals):
            if val > 0:
                ax_total.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                            f'{val:,}', ha='center', va='bottom', fontsize=9, fontweight='bold')
        ax_total.set_ylabel('Total Eksekusi')
        ax_total.grid(True, axis='y', alpha=0.3)
    else:
        ax_total.text(0.5, 0.5, 'Belum ada data', ha='center', va='center',
                    transform=ax_total.transAxes, color='gray')

    # -- 7. Starvation Analysis (bawah tengah) --
    ax_starv = fig.add_subplot(gs[2, 1])
    ax_starv.set_title('Analisis Starvation (Fase 3 vs Fase 1)', fontweight='bold', fontsize=12)

    counts_p1 = store.get_phase_worker_counts(1)
    counts_p3 = store.get_phase_worker_counts(3)
    total_p1 = sum(counts_p1) if sum(counts_p1) > 0 else 1
    total_p3 = sum(counts_p3) if sum(counts_p3) > 0 else 1

    if any(c > 0 for c in counts_p1) or any(c > 0 for c in counts_p3):
        x_starv = np.arange(NUM_WORKERS)
        width_starv = 0.35

        pct_p1 = [c / total_p1 * 100 for c in counts_p1]
        pct_p3 = [c / total_p3 * 100 for c in counts_p3]

        ax_starv.bar(x_starv - width_starv/2, pct_p1, width_starv,
                    label='Fase 1 (Preemptive)', color='#27ae60', alpha=0.8)
        ax_starv.bar(x_starv + width_starv/2, pct_p3, width_starv,
                    label='Fase 3 (CPU Hog)', color='#e74c3c', alpha=0.8)

        ax_starv.set_xticks(x_starv)
        ax_starv.set_xticklabels(WORKER_NAMES)
        ax_starv.set_ylabel('Persentase CPU (%)')
        ax_starv.set_ylim(0, 110)
        ax_starv.legend(fontsize=9)
        ax_starv.grid(True, axis='y', alpha=0.3)

        # Garis ideal
        ax_starv.axhline(y=100/NUM_WORKERS, color='blue', linestyle=':', alpha=0.5,
                        label=f'Ideal ({100/NUM_WORKERS:.1f}%)')
    else:
        ax_starv.text(0.5, 0.5, 'Belum ada data', ha='center', va='center',
                    transform=ax_starv.transAxes, color='gray')

    # -- 8. Ringkasan & Kesimpulan (bawah kanan) --
    ax_info = fig.add_subplot(gs[2, 2])
    ax_info.axis('off')
    ax_info.set_title('Ringkasan & Kesimpulan', fontweight='bold', fontsize=12)

    info_lines = []
    info_lines.append("PERBANDINGAN TIGA METODE:")
    info_lines.append("")

    for ph in phases:
        if ph in store.phase_summary:
            ps = store.phase_summary[ph]
            info_lines.append(f"Fase {ph} ({PHASE_SHORT_NAMES[ph]}):")
            info_lines.append(f"  Total : {ps['total_count']:,}")
            info_lines.append(f"  Fair  : {ps['fairness']:.4f}")
            info_lines.append("")

    if store.phase_summary:
        info_lines.append("KESIMPULAN:")
        f1 = store.phase_summary.get(1, {}).get('fairness', 0)
        f2 = store.phase_summary.get(2, {}).get('fairness', 0)
        f3 = store.phase_summary.get(3, {}).get('fairness', 0)

        if f1 > 0.9:
            info_lines.append("• Preemptive: Adil otomatis")
        if f2 > 0.9:
            info_lines.append("• Cooperative: Yield efektif")
        if f3 < 0.8:
            info_lines.append("• CPU Hog: STARVATION!")
    else:
        info_lines.append("Belum ada data ringkasan")

    info_text = '\n'.join(info_lines)
    ax_info.text(0.05, 0.95, info_text, transform=ax_info.transAxes,
                fontsize=9, verticalalignment='top', fontfamily='monospace',
                bbox=dict(boxstyle='round', facecolor='#ecf0f1', alpha=0.8))

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"cooperative_analysis_{timestamp}.png"
    plt.savefig(filename, dpi=150, bbox_inches='tight', facecolor='white')
    print(f"\n[OK] Grafik disimpan: {filename}")
    plt.close()


# ==========================================================================
# MODE DEMO — Data Simulasi
# ==========================================================================

def run_demo():
    """Jalankan mode demo dengan data simulasi."""
    print("=" * 60)
    print("  MODE DEMO — STM32_12_Task_Cooperative")
    print("  Menggunakan data simulasi")
    print("=" * 60)
    print()

    store = CooperativeDataStore()

    # -- Simulasi Fase 1: Preemptive + Time Slicing --
    print("--- Simulasi Fase 1: Preemptive ---")
    # Dengan vTaskDelay, distribusi sangat merata
    sim_counts_p1 = [195, 198, 192]  # Sangat merata
    for wid, count in enumerate(sim_counts_p1):
        for step in range(1, 11):
            c = int(count * step / 10)
            line = f"[DATA]WORKER id={wid},count={c},phase=1"
            parse_line(line, store)
            if step % 3 == 0:
                print(f"  {line}")
            time.sleep(0.01)

    total_p1 = sum(sim_counts_p1)
    fair_p1 = calculate_jain(sim_counts_p1)
    line = f"[DATA]PHASE phase=1,total_count={total_p1},fairness={fair_p1:.4f}"
    parse_line(line, store)
    print(f"  {line}")
    print()

    # -- Simulasi Fase 2: Cooperative Yield --
    print("--- Simulasi Fase 2: Cooperative Yield ---")
    # Dengan taskYIELD, distribusi cukup merata tapi lebih banyak total
    sim_counts_p2 = [12450, 12380, 12520]
    for wid, count in enumerate(sim_counts_p2):
        for step in range(1, 11):
            c = int(count * step / 10)
            line = f"[DATA]WORKER id={wid},count={c},phase=2"
            parse_line(line, store)
            if step % 3 == 0:
                print(f"  {line}")
            time.sleep(0.01)

    total_p2 = sum(sim_counts_p2)
    fair_p2 = calculate_jain(sim_counts_p2)
    line = f"[DATA]PHASE phase=2,total_count={total_p2},fairness={fair_p2:.4f}"
    parse_line(line, store)
    print(f"  {line}")
    print()

    # -- Simulasi Fase 3: CPU Hog --
    print("--- Simulasi Fase 3: CPU Hog ---")
    # Worker-0 monopoli, sisanya mendapat sedikit dari time slicing
    sim_counts_p3 = [18200, 3150, 3080]
    for wid, count in enumerate(sim_counts_p3):
        for step in range(1, 11):
            c = int(count * step / 10)
            line = f"[DATA]WORKER id={wid},count={c},phase=3"
            parse_line(line, store)
            if step % 3 == 0:
                print(f"  {line}")
            time.sleep(0.01)

    total_p3 = sum(sim_counts_p3)
    fair_p3 = calculate_jain(sim_counts_p3)
    line = f"[DATA]PHASE phase=3,total_count={total_p3},fairness={fair_p3:.4f}"
    parse_line(line, store)
    print(f"  {line}")
    print()

    # Buat visualisasi
    print("[INFO] Membuat visualisasi...")
    create_visualization(store)

    # Cetak analisis
    print("\n" + "=" * 60)
    print("  ANALISIS HASIL DEMO")
    print("=" * 60)
    print(f"\n  Fase 1 (Preemptive):")
    print(f"    Total: {total_p1:>8,}  |  Fairness: {fair_p1:.4f}")
    print(f"    → Distribusi merata berkat time slicing")

    print(f"\n  Fase 2 (Cooperative Yield):")
    print(f"    Total: {total_p2:>8,}  |  Fairness: {fair_p2:.4f}")
    print(f"    → Lebih banyak eksekusi karena tanpa delay")
    print(f"    → taskYIELD() efektif membagi CPU")

    print(f"\n  Fase 3 (CPU Hog):")
    print(f"    Total: {total_p3:>8,}  |  Fairness: {fair_p3:.4f}")
    pct_hog = sim_counts_p3[0] / total_p3 * 100
    print(f"    → Worker-0 monopoli {pct_hog:.1f}% CPU!")
    print(f"    → Worker-1 & Worker-2 KELAPARAN (starvation)")
    print()
    print("=" * 60)


def calculate_jain(counts):
    """Hitung Jain's Fairness Index."""
    n = len(counts)
    if n == 0:
        return 0.0
    s = sum(counts)
    ss = sum(c * c for c in counts)
    if ss == 0:
        return 1.0
    return (s * s) / (n * ss)


# ==========================================================================
# MODE SERIAL
# ==========================================================================

def run_serial(port, baudrate=115200):
    """Baca data dari port serial dan visualisasikan."""
    try:
        import serial
    except ImportError:
        print("[ERROR] pyserial tidak tersedia. Install: pip install pyserial")
        sys.exit(1)

    print(f"[INFO] Membuka port serial {port} @ {baudrate} baud...")

    store = CooperativeDataStore()
    line_count = 0
    data_count = 0

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Port serial {port} terbuka")
        print("[INFO] Tekan Ctrl+C untuk berhenti dan membuat grafik\n")

        while True:
            raw = ser.readline()
            if raw:
                try:
                    line = raw.decode('utf-8', errors='replace').strip()
                except Exception:
                    continue

                if not line:
                    continue

                line_count += 1
                result = parse_line(line, store)

                if result:
                    data_count += 1

                # Tampilkan baris relevan
                if '[DATA]' in line or '[INFO]' in line or 'FASE' in line:
                    print(f"[{line_count:>6}] {line}")

    except KeyboardInterrupt:
        print(f"\n\n[INFO] Dihentikan oleh pengguna")
        print(f"[INFO] Total baris: {line_count}, Data tags: {data_count}")

        if data_count > 0:
            print("[INFO] Membuat visualisasi...")
            create_visualization(store)
        else:
            print("[WARN] Tidak ada data untuk divisualisasikan")

    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
        sys.exit(1)

    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"[OK] Port serial ditutup")


# ==========================================================================
# MODE FILE
# ==========================================================================

def run_file(filepath):
    """Parse data dari file log."""
    print(f"[INFO] Membaca file: {filepath}")

    store = CooperativeDataStore()
    line_count = 0
    data_count = 0

    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue

                line_count += 1
                result = parse_line(line, store)

                if result:
                    data_count += 1

                if '[DATA]' in line:
                    print(f"  [{line_count:>6}] {line}")

    except FileNotFoundError:
        print(f"[ERROR] File tidak ditemukan: {filepath}")
        sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Gagal membaca file: {e}")
        sys.exit(1)

    print(f"\n[INFO] Total baris: {line_count}, Data tags: {data_count}")

    if data_count > 0:
        print("[INFO] Membuat visualisasi...")
        create_visualization(store)
    else:
        print("[WARN] Tidak ada data [DATA] ditemukan dalam file")


# ==========================================================================
# MAIN
# ==========================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Debug Script: STM32_12_Task_Cooperative',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  %(prog)s --demo                   Mode demo dengan data simulasi
  %(prog)s --port /dev/ttyUSB0      Baca dari serial port
  %(prog)s --port COM3              Baca dari serial port (Windows)
  %(prog)s --file output.log        Parse dari file log
        """
    )
    parser.add_argument('--port', type=str, help='Serial port (misal: /dev/ttyUSB0, COM3)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--file', type=str, help='Path ke file log')
    parser.add_argument('--demo', action='store_true', help='Jalankan mode demo')

    args = parser.parse_args()

    if args.demo:
        run_demo()
    elif args.port:
        run_serial(args.port, args.baud)
    elif args.file:
        run_file(args.file)
    else:
        print("[INFO] Tidak ada argumen. Menjalankan mode demo...\n")
        run_demo()


if __name__ == '__main__':
    main()

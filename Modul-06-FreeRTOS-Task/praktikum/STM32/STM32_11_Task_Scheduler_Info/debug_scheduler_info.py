#!/usr/bin/env python3
"""
============================================================================
Debug Script: STM32_11_Task_Scheduler_Info
============================================================================
Parser & Visualisasi data dari STM32_11_Task_Scheduler_Info

Fitur:
  - Parse output [DATA]TASK dan [DATA]SYSTEM dari serial UART
  - Tabel real-time informasi task (nama, state, prioritas, stack HWM)
  - Grafik batang prioritas dan stack usage per task
  - Grafik pie distribusi state task
  - Trend free heap dan jumlah task seiring waktu
  - Mode demo dengan data simulasi

Penggunaan:
  python debug_scheduler_info.py --port COM3
  python debug_scheduler_info.py --file output.log
  python debug_scheduler_info.py --demo

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
PATTERN_TASK = re.compile(
    r'\[DATA\]TASK\s+name=([^,]+),state=([^,]+),priority=(\d+),stack=(\d+),number=(\d+)'
)
PATTERN_SYSTEM = re.compile(
    r'\[DATA\]SYSTEM\s+total_tasks=(\d+),free_heap=(\d+),min_heap=(\d+)'
)

# Warna untuk state task
STATE_COLORS = {
    'Running':   '#2ecc71',
    'Ready':     '#3498db',
    'Blocked':   '#f39c12',
    'Suspended': '#e74c3c',
    'Deleted':   '#95a5a6',
    'Unknown':   '#bdc3c7',
}

# ==========================================================================
# KELAS DATA STORE
# ==========================================================================

class SchedulerDataStore:
    """Penyimpanan data scheduler yang di-parse dari serial output."""

    def __init__(self):
        self.tasks = {}             # {name: {state, priority, stack, number}}
        self.system_history = []    # [{timestamp, total_tasks, free_heap, min_heap}]
        self.task_snapshots = []    # [{timestamp, tasks: {name: {state, ...}}}]
        self.state_counts = defaultdict(int)
        self.start_time = time.time()

    def update_task(self, name, state, priority, stack, number):
        """Perbarui data task."""
        self.tasks[name] = {
            'state': state,
            'priority': int(priority),
            'stack': int(stack),
            'number': int(number),
            'timestamp': time.time() - self.start_time,
        }

    def update_system(self, total_tasks, free_heap, min_heap):
        """Perbarui data sistem."""
        entry = {
            'timestamp': time.time() - self.start_time,
            'total_tasks': int(total_tasks),
            'free_heap': int(free_heap),
            'min_heap': int(min_heap),
        }
        self.system_history.append(entry)

    def take_snapshot(self):
        """Ambil snapshot task saat ini."""
        snapshot = {
            'timestamp': time.time() - self.start_time,
            'tasks': dict(self.tasks),
        }
        self.task_snapshots.append(snapshot)

        # Hitung distribusi state
        self.state_counts.clear()
        for t in self.tasks.values():
            self.state_counts[t['state']] += 1

    def get_task_names(self):
        """Dapatkan daftar nama task."""
        return sorted(self.tasks.keys())

    def get_state_distribution(self):
        """Dapatkan distribusi state task."""
        dist = defaultdict(int)
        for t in self.tasks.values():
            dist[t['state']] += 1
        return dict(dist)


# ==========================================================================
# FUNGSI PARSING
# ==========================================================================

def parse_line(line, store):
    """Parse satu baris output serial."""
    line = line.strip()

    # Parse [DATA]TASK
    match = PATTERN_TASK.search(line)
    if match:
        name, state, priority, stack, number = match.groups()
        store.update_task(name, state, priority, stack, number)
        return 'task'

    # Parse [DATA]SYSTEM
    match = PATTERN_SYSTEM.search(line)
    if match:
        total_tasks, free_heap, min_heap = match.groups()
        store.update_system(total_tasks, free_heap, min_heap)
        store.take_snapshot()
        return 'system'

    return None


# ==========================================================================
# FUNGSI VISUALISASI
# ==========================================================================

def create_visualization(store):
    """Buat visualisasi lengkap dari data scheduler."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        from matplotlib.gridspec import GridSpec
    except ImportError:
        print("[ERROR] matplotlib tidak tersedia. Install: pip install matplotlib")
        return

    fig = plt.figure(figsize=(18, 14))
    fig.suptitle('STM32_11 — Task Scheduler Info Dashboard',
                 fontsize=16, fontweight='bold', y=0.98)
    gs = GridSpec(3, 3, figure=fig, hspace=0.35, wspace=0.30)

    # -- 1. Tabel Informasi Task (atas kiri, span 2 kolom) --
    ax_table = fig.add_subplot(gs[0, :2])
    ax_table.axis('off')
    ax_table.set_title('Informasi Task Saat Ini', fontweight='bold', fontsize=12)

    if store.tasks:
        headers = ['Nama', 'State', 'Prioritas', 'Stack HWM', 'No']
        rows = []
        cell_colors = []
        for name in sorted(store.tasks.keys()):
            t = store.tasks[name]
            rows.append([name, t['state'], str(t['priority']),
                        str(t['stack']), str(t['number'])])
            color = STATE_COLORS.get(t['state'], '#ffffff')
            cell_colors.append(['#f8f9fa', color + '40', '#f8f9fa', '#f8f9fa', '#f8f9fa'])

        table = ax_table.table(cellText=rows, colLabels=headers,
                               cellColours=cell_colors,
                               loc='center', cellLoc='center')
        table.auto_set_font_size(False)
        table.set_fontsize(9)
        table.scale(1, 1.4)

        for (row, col), cell in table.get_celld().items():
            if row == 0:
                cell.set_facecolor('#2c3e50')
                cell.set_text_props(color='white', fontweight='bold')
    else:
        ax_table.text(0.5, 0.5, 'Belum ada data task',
                     ha='center', va='center', fontsize=12, color='gray')

    # -- 2. Distribusi State (atas kanan, pie chart) --
    ax_pie = fig.add_subplot(gs[0, 2])
    ax_pie.set_title('Distribusi State Task', fontweight='bold', fontsize=12)

    state_dist = store.get_state_distribution()
    if state_dist:
        labels = list(state_dist.keys())
        sizes = list(state_dist.values())
        colors = [STATE_COLORS.get(s, '#bdc3c7') for s in labels]
        wedges, texts, autotexts = ax_pie.pie(sizes, labels=labels, colors=colors,
                                               autopct='%1.0f%%', startangle=90,
                                               textprops={'fontsize': 9})
        for t in autotexts:
            t.set_fontweight('bold')
    else:
        ax_pie.text(0.5, 0.5, 'Belum ada data', ha='center', va='center', color='gray')

    # -- 3. Prioritas per Task (tengah kiri) --
    ax_prio = fig.add_subplot(gs[1, 0])
    ax_prio.set_title('Prioritas Task', fontweight='bold', fontsize=12)

    if store.tasks:
        names = sorted(store.tasks.keys())
        priorities = [store.tasks[n]['priority'] for n in names]
        colors_bar = ['#3498db' if store.tasks[n]['state'] != 'Suspended'
                      else '#e74c3c' for n in names]
        bars = ax_prio.barh(names, priorities, color=colors_bar, edgecolor='white')
        ax_prio.set_xlabel('Prioritas')
        ax_prio.set_xlim(0, max(priorities) + 1)
        for bar, val in zip(bars, priorities):
            ax_prio.text(bar.get_width() + 0.1, bar.get_y() + bar.get_height()/2,
                        str(val), va='center', fontweight='bold', fontsize=9)
    else:
        ax_prio.text(0.5, 0.5, 'Belum ada data', ha='center', va='center',
                    transform=ax_prio.transAxes, color='gray')

    # -- 4. Stack High Water Mark per Task (tengah tengah) --
    ax_stack = fig.add_subplot(gs[1, 1])
    ax_stack.set_title('Stack High Water Mark (words)', fontweight='bold', fontsize=12)

    if store.tasks:
        names = sorted(store.tasks.keys())
        stacks = [store.tasks[n]['stack'] for n in names]
        colors_stack = ['#2ecc71' if s > 50 else '#e74c3c' for s in stacks]
        bars = ax_stack.barh(names, stacks, color=colors_stack, edgecolor='white')
        ax_stack.set_xlabel('Stack HWM (words)')
        for bar, val in zip(bars, stacks):
            ax_stack.text(bar.get_width() + 1, bar.get_y() + bar.get_height()/2,
                        str(val), va='center', fontweight='bold', fontsize=9)
    else:
        ax_stack.text(0.5, 0.5, 'Belum ada data', ha='center', va='center',
                    transform=ax_stack.transAxes, color='gray')

    # -- 5. Jumlah Task dari Waktu ke Waktu (tengah kanan) --
    ax_count = fig.add_subplot(gs[1, 2])
    ax_count.set_title('Jumlah Task vs Waktu', fontweight='bold', fontsize=12)

    if store.system_history:
        times = [e['timestamp'] for e in store.system_history]
        counts = [e['total_tasks'] for e in store.system_history]
        ax_count.plot(times, counts, 'o-', color='#8e44ad', linewidth=2,
                     markersize=5, label='Total Tasks')
        ax_count.fill_between(times, counts, alpha=0.2, color='#8e44ad')
        ax_count.set_xlabel('Waktu (detik)')
        ax_count.set_ylabel('Jumlah Task')
        ax_count.legend(fontsize=9)
        ax_count.grid(True, alpha=0.3)
    else:
        ax_count.text(0.5, 0.5, 'Belum ada data', ha='center', va='center',
                    transform=ax_count.transAxes, color='gray')

    # -- 6. Free Heap vs Waktu (bawah kiri, span 2 kolom) --
    ax_heap = fig.add_subplot(gs[2, :2])
    ax_heap.set_title('Free Heap Memory vs Waktu', fontweight='bold', fontsize=12)

    if store.system_history:
        times = [e['timestamp'] for e in store.system_history]
        free_heap = [e['free_heap'] for e in store.system_history]
        min_heap = [e['min_heap'] for e in store.system_history]

        ax_heap.plot(times, free_heap, '-', color='#27ae60', linewidth=2,
                    label='Free Heap', marker='o', markersize=4)
        ax_heap.plot(times, min_heap, '--', color='#c0392b', linewidth=2,
                    label='Min Free Heap', marker='s', markersize=4)
        ax_heap.fill_between(times, min_heap, free_heap, alpha=0.15, color='#27ae60')
        ax_heap.set_xlabel('Waktu (detik)')
        ax_heap.set_ylabel('Bytes')
        ax_heap.legend(fontsize=9)
        ax_heap.grid(True, alpha=0.3)
    else:
        ax_heap.text(0.5, 0.5, 'Belum ada data', ha='center', va='center',
                    transform=ax_heap.transAxes, color='gray')

    # -- 7. Info Ringkasan (bawah kanan) --
    ax_info = fig.add_subplot(gs[2, 2])
    ax_info.axis('off')
    ax_info.set_title('Ringkasan Sistem', fontweight='bold', fontsize=12)

    info_text = ""
    if store.system_history:
        latest = store.system_history[-1]
        info_text += f"Total Task    : {latest['total_tasks']}\n"
        info_text += f"Free Heap     : {latest['free_heap']} bytes\n"
        info_text += f"Min Free Heap : {latest['min_heap']} bytes\n"
        info_text += f"Snapshots     : {len(store.system_history)}\n"
        info_text += f"\nTask States:\n"
        for state, count in sorted(store.get_state_distribution().items()):
            info_text += f"  {state}: {count}\n"
    else:
        info_text = "Belum ada data sistem"

    ax_info.text(0.1, 0.9, info_text, transform=ax_info.transAxes,
                fontsize=10, verticalalignment='top', fontfamily='monospace',
                bbox=dict(boxstyle='round', facecolor='#ecf0f1', alpha=0.8))

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"scheduler_info_{timestamp}.png"
    plt.savefig(filename, dpi=150, bbox_inches='tight', facecolor='white')
    print(f"\n[OK] Grafik disimpan: {filename}")
    plt.close()


# ==========================================================================
# MODE DEMO — Data Simulasi
# ==========================================================================

def run_demo():
    """Jalankan mode demo dengan data simulasi."""
    print("=" * 60)
    print("  MODE DEMO — STM32_11_Task_Scheduler_Info")
    print("  Menggunakan data simulasi")
    print("=" * 60)
    print()

    store = SchedulerDataStore()

    # Simulasi beberapa siklus data
    demo_scenarios = [
        # Siklus 1: Normal — semua task aktif
        {
            'tasks': [
                ('Worker-0', 'Blocked',  2, 180, 1),
                ('Worker-1', 'Blocked',  2, 165, 2),
                ('Worker-2', 'Blocked',  2, 172, 3),
                ('Monitor',  'Running',  3, 310, 4),
                ('IDLE',     'Ready',    0, 105, 5),
                ('Tmr Svc',  'Blocked',  3,  98, 6),
            ],
            'system': (6, 4280, 3960),
        },
        # Siklus 2: Worker-1 suspended
        {
            'tasks': [
                ('Worker-0', 'Blocked',   2, 175, 1),
                ('Worker-1', 'Suspended', 2, 165, 2),
                ('Worker-2', 'Blocked',   2, 168, 3),
                ('Monitor',  'Running',   3, 305, 4),
                ('IDLE',     'Ready',     0, 105, 5),
                ('Tmr Svc',  'Blocked',   3,  98, 6),
            ],
            'system': (6, 4280, 3960),
        },
        # Siklus 3: Task temporer ditambahkan
        {
            'tasks': [
                ('Worker-0', 'Blocked',  2, 172, 1),
                ('Worker-1', 'Blocked',  2, 162, 2),
                ('Worker-2', 'Blocked',  2, 165, 3),
                ('Monitor',  'Running',  3, 300, 4),
                ('IDLE',     'Ready',    0, 105, 5),
                ('Tmr Svc',  'Blocked',  3,  98, 6),
                ('TempTask', 'Blocked',  1, 140, 7),
            ],
            'system': (7, 3580, 3580),
        },
        # Siklus 4: Task temporer dihapus, kembali normal
        {
            'tasks': [
                ('Worker-0', 'Blocked',  2, 170, 1),
                ('Worker-1', 'Blocked',  2, 160, 2),
                ('Worker-2', 'Ready',    2, 163, 3),
                ('Monitor',  'Running',  3, 298, 4),
                ('IDLE',     'Ready',    0, 105, 5),
                ('Tmr Svc',  'Blocked',  3,  98, 6),
            ],
            'system': (6, 4100, 3580),
        },
        # Siklus 5: Variasi state
        {
            'tasks': [
                ('Worker-0', 'Ready',    2, 168, 1),
                ('Worker-1', 'Blocked',  2, 158, 2),
                ('Worker-2', 'Blocked',  2, 161, 3),
                ('Monitor',  'Running',  3, 295, 4),
                ('IDLE',     'Ready',    0, 105, 5),
                ('Tmr Svc',  'Blocked',  3,  98, 6),
            ],
            'system': (6, 4100, 3580),
        },
    ]

    for i, scenario in enumerate(demo_scenarios):
        # Simulasi jeda waktu
        store.start_time = time.time() - (i * 5)

        print(f"--- Siklus {i + 1} ---")

        for name, state, prio, stack, num in scenario['tasks']:
            line = f"[DATA]TASK name={name},state={state},priority={prio},stack={stack},number={num}"
            parse_line(line, store)
            print(f"  {line}")

        total, free, minf = scenario['system']
        line = f"[DATA]SYSTEM total_tasks={total},free_heap={free},min_heap={minf}"
        parse_line(line, store)
        print(f"  {line}")
        print()

        # Tambah sedikit delay untuk timestamp berbeda
        time.sleep(0.1)

    # Buat visualisasi
    print("\n[INFO] Membuat visualisasi...")
    create_visualization(store)

    # Cetak ringkasan
    print("\n" + "=" * 60)
    print("  RINGKASAN DATA DEMO")
    print("=" * 60)
    print(f"  Total task terpantau  : {len(store.tasks)}")
    print(f"  Total snapshots       : {len(store.system_history)}")
    print(f"  Distribusi state      :")
    for state, count in sorted(store.get_state_distribution().items()):
        print(f"    {state:12s}: {count}")
    print("=" * 60)


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

    store = SchedulerDataStore()
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

                # Tampilkan baris yang relevan
                if '[DATA]' in line or '[INFO]' in line or '[ERROR]' in line:
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

    store = SchedulerDataStore()
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
        description='Debug Script: STM32_11_Task_Scheduler_Info',
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

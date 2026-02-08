#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_06_Task_Stack_Monitor
==========================================================================
 Mem-parse output serial dari program Stack Monitor STM32 FreeRTOS.
 Mengekstrak data [DATA] tag, memvisualisasikan penggunaan stack
 per-task dari waktu ke waktu, dan menampilkan peringatan stack.

 Penggunaan:
   python debug_stack_monitor.py --port /dev/ttyUSB0
   python debug_stack_monitor.py --file capture.log
   python debug_stack_monitor.py --port COM3 --baud 115200

 Output:
   - Grafik real-time penggunaan stack per task
   - Bar chart perbandingan stack allocation vs usage
   - Timeline fase demonstrasi
   - Rekomendasi ukuran stack
==========================================================================
"""

import argparse
import sys
import re
import time
import os
from datetime import datetime
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.patches import FancyBboxPatch
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


# =========================================================================
# Kelas utama parser data stack monitor
# =========================================================================
class StackMonitorParser:
    """Parser dan visualizer untuk data stack monitoring FreeRTOS."""

    def __init__(self):
        """Inisialisasi parser dengan struktur data kosong."""
        # Data stack per task: {task_name: [(tick, alloc, hwm, used, pct)]}
        self.stack_data = defaultdict(list)
        # Data heap: [(tick, free_bytes)]
        self.heap_data = []
        # Data fase: [(tick, phase)]
        self.phase_data = []
        # Data peringatan: [(tick, task, pct)]
        self.warnings = []
        # Data rekomendasi: {task: (current, recommended)}
        self.recommendations = {}
        # Data counter task: {task: [(tick, count)]}
        self.task_counters = defaultdict(list)
        # Event log
        self.events = []
        # Konfigurasi awal
        self.init_config = {}
        # Status
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial dan ekstrak data [DATA]."""
        self.total_lines += 1
        line = line.strip()

        if not line:
            return

        # Cek tag [DATA]
        data_match = re.match(r'\[DATA\]\s+(\w+),(.*)', line)
        if not data_match:
            # Cetak baris non-data untuk monitoring
            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
            print(f"[{timestamp}] {line}")
            return

        self.data_lines += 1
        tag = data_match.group(1)
        params_str = data_match.group(2)

        # Parse parameter key=value
        params = {}
        for kv in re.findall(r'(\w+)=([^,]+)', params_str):
            key, val = kv
            try:
                params[key] = int(val)
            except ValueError:
                try:
                    params[key] = float(val)
                except ValueError:
                    params[key] = val

        # Proses berdasarkan tag
        self._process_tag(tag, params, line)

    def _process_tag(self, tag, params, raw_line):
        """Proses data berdasarkan tag."""
        timestamp = datetime.now().strftime('%H:%M:%S')

        if tag == 'INIT':
            self.init_config = params
            print(f"\n{'='*60}")
            print(f" INISIALISASI STACK MONITOR")
            print(f"{'='*60}")
            for k, v in params.items():
                print(f"  {k}: {v} words ({v * 4} bytes)")
            print(f"{'='*60}\n")

        elif tag == 'STACK':
            task = params.get('task', 'unknown')
            alloc = params.get('alloc', 0)
            hwm = params.get('hwm', 0)
            used = params.get('used', 0)
            pct = params.get('pct', 0)
            self.stack_data[task].append({
                'time': time.time() - self.start_time,
                'alloc': alloc, 'hwm': hwm,
                'used': used, 'pct': pct
            })
            bar = '#' * (pct // 5) + '.' * (20 - pct // 5)
            status = "⚠️ " if pct >= 80 else "✓ "
            print(f"  {status}{task:12s}: [{bar}] {pct:3d}% "
                  f"({used}/{alloc} words)")

        elif tag == 'STACK_WARN':
            task = params.get('task', 'unknown')
            pct = params.get('pct', 0)
            self.warnings.append({
                'time': time.time() - self.start_time,
                'task': task, 'pct': pct
            })
            print(f"  ⚠️  PERINGATAN: {task} penggunaan {pct}%!")

        elif tag == 'HEAP':
            free_heap = params.get('free', 0)
            tick = params.get('tick', 0)
            self.heap_data.append({
                'time': time.time() - self.start_time,
                'free': free_heap, 'tick': tick
            })

        elif tag == 'PHASE':
            phase = int(params_str.split(',')[0]) if ',' in (raw_line) else 0
            phase_match = re.search(r'PHASE,(\d+)', raw_line)
            if phase_match:
                phase = int(phase_match.group(1))
            self.phase_data.append({
                'time': time.time() - self.start_time,
                'phase': phase
            })
            print(f"\n{'='*60}")
            print(f" FASE {phase}")
            print(f"{'='*60}\n")

        elif tag in ('SMALL', 'MEDIUM', 'LARGE'):
            cnt = params.get('cnt', 0)
            tick = params.get('tick', 0)
            phase = params.get('phase', 0)
            self.task_counters[tag].append({
                'time': time.time() - self.start_time,
                'cnt': cnt, 'tick': tick, 'phase': phase
            })

        elif tag == 'MONITOR':
            cycle = params.get('cycle', 0)
            warns = params.get('warnings', 0)
            phase = params.get('phase', 0)
            print(f"\n  Monitor siklus {cycle} (fase {phase}), "
                  f"peringatan: {warns}")

        elif tag == 'RECOMMEND':
            task = params.get('task', 'unknown')
            current = params.get('current', 0)
            recommended = params.get('recommended', 0)
            self.recommendations[task] = {
                'current': current, 'recommended': recommended
            }
            print(f"  📋 {task}: {current} → {recommended} words")

        elif tag == 'STACK_OVERFLOW':
            task = params.get('task', 'unknown')
            print(f"\n  🔴 STACK OVERFLOW pada {task}!")
            self.events.append({
                'time': time.time() - self.start_time,
                'event': f'OVERFLOW: {task}'
            })

        elif tag == 'MALLOC_FAIL':
            print(f"\n  🔴 MALLOC FAILED!")
            self.events.append({
                'time': time.time() - self.start_time,
                'event': 'MALLOC_FAIL'
            })

        elif tag == 'SUMMARY_COMPLETE':
            print(f"\n{'='*60}")
            print(f" SUMMARY SELESAI")
            print(f"{'='*60}\n")

    def plot_results(self):
        """Buat visualisasi matplotlib dari data yang dikumpulkan."""
        if not HAS_MATPLOTLIB:
            print("\n[WARN] matplotlib tidak tersedia, skip visualisasi")
            return

        if not self.stack_data:
            print("\n[WARN] Tidak ada data stack untuk divisualisasikan")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('STM32 FreeRTOS Stack Monitor Analysis',
                     fontsize=14, fontweight='bold')

        # --- Plot 1: Stack usage percentage over time ---
        ax1 = axes[0, 0]
        colors = {'SmallTask': '#e74c3c', 'MediumTask': '#3498db',
                  'LargeTask': '#2ecc71', 'MonitorTask': '#9b59b6'}

        for task_name, data_list in self.stack_data.items():
            if data_list:
                times = [d['time'] for d in data_list]
                pcts = [d['pct'] for d in data_list]
                color = colors.get(task_name, '#95a5a6')
                ax1.plot(times, pcts, '-o', label=task_name,
                         color=color, markersize=4)

        ax1.axhline(y=80, color='red', linestyle='--',
                    alpha=0.7, label='Batas Peringatan (80%)')
        ax1.set_xlabel('Waktu (detik)')
        ax1.set_ylabel('Penggunaan Stack (%)')
        ax1.set_title('Penggunaan Stack vs Waktu')
        ax1.legend(loc='upper left', fontsize=8)
        ax1.set_ylim(0, 105)
        ax1.grid(True, alpha=0.3)

        # Tambahkan region fase
        for pd_item in self.phase_data:
            ax1.axvline(x=pd_item['time'], color='gray',
                        linestyle=':', alpha=0.5)

        # --- Plot 2: Stack allocation comparison (bar chart) ---
        ax2 = axes[0, 1]
        task_names = list(self.stack_data.keys())
        if task_names:
            last_alloc = []
            last_used = []
            last_hwm = []
            bar_colors = []

            for tn in task_names:
                if self.stack_data[tn]:
                    last = self.stack_data[tn][-1]
                    last_alloc.append(last['alloc'])
                    last_used.append(last['used'])
                    last_hwm.append(last['hwm'])
                    bar_colors.append(colors.get(tn, '#95a5a6'))

            x = range(len(task_names))
            width = 0.35
            bars1 = ax2.bar([i - width/2 for i in x], last_alloc,
                            width, label='Dialokasikan', color='lightblue',
                            edgecolor='navy')
            bars2 = ax2.bar([i + width/2 for i in x], last_used,
                            width, label='Terpakai', color=bar_colors,
                            edgecolor='black')

            ax2.set_xlabel('Task')
            ax2.set_ylabel('Stack (words)')
            ax2.set_title('Alokasi vs Penggunaan Stack')
            ax2.set_xticks(list(x))
            ax2.set_xticklabels(task_names, rotation=15, fontsize=8)
            ax2.legend()
            ax2.grid(True, alpha=0.3, axis='y')

        # --- Plot 3: Heap usage over time ---
        ax3 = axes[1, 0]
        if self.heap_data:
            h_times = [d['time'] for d in self.heap_data]
            h_free = [d['free'] for d in self.heap_data]
            ax3.plot(h_times, h_free, '-s', color='#e67e22',
                     markersize=4, label='Free Heap')
            ax3.fill_between(h_times, h_free, alpha=0.3, color='#e67e22')
            ax3.set_xlabel('Waktu (detik)')
            ax3.set_ylabel('Free Heap (bytes)')
            ax3.set_title('Free Heap vs Waktu')
            ax3.legend()
            ax3.grid(True, alpha=0.3)

        # --- Plot 4: Recommendations ---
        ax4 = axes[1, 1]
        if self.recommendations:
            rec_tasks = list(self.recommendations.keys())
            rec_curr = [self.recommendations[t]['current'] for t in rec_tasks]
            rec_reco = [self.recommendations[t]['recommended'] for t in rec_tasks]

            x = range(len(rec_tasks))
            width = 0.35
            ax4.bar([i - width/2 for i in x], rec_curr, width,
                    label='Saat Ini', color='#3498db')
            ax4.bar([i + width/2 for i in x], rec_reco, width,
                    label='Rekomendasi', color='#2ecc71')
            ax4.set_xlabel('Task')
            ax4.set_ylabel('Stack Size (words)')
            ax4.set_title('Rekomendasi Ukuran Stack')
            ax4.set_xticks(list(x))
            ax4.set_xticklabels(rec_tasks, rotation=15, fontsize=8)
            ax4.legend()
            ax4.grid(True, alpha=0.3, axis='y')
        else:
            ax4.text(0.5, 0.5, 'Menunggu data rekomendasi...',
                     ha='center', va='center', fontsize=12,
                     transform=ax4.transAxes)
            ax4.set_title('Rekomendasi Ukuran Stack')

        plt.tight_layout()
        plt.savefig('stack_monitor_analysis.png', dpi=150,
                    bbox_inches='tight')
        print(f"\n[INFO] Grafik disimpan: stack_monitor_analysis.png")
        plt.show()

    def print_summary(self):
        """Cetak ringkasan akhir dari data yang dikumpulkan."""
        elapsed = time.time() - self.start_time
        print(f"\n{'='*60}")
        print(f" RINGKASAN PARSING")
        print(f"{'='*60}")
        print(f"  Durasi       : {elapsed:.1f} detik")
        print(f"  Total baris  : {self.total_lines}")
        print(f"  Data baris   : {self.data_lines}")
        print(f"  Tasks tracked: {len(self.stack_data)}")
        print(f"  Heap samples : {len(self.heap_data)}")
        print(f"  Peringatan   : {len(self.warnings)}")

        # Statistik per task
        if self.stack_data:
            print(f"\n  Stack Usage (terakhir):")
            for task, data_list in self.stack_data.items():
                if data_list:
                    last = data_list[-1]
                    max_pct = max(d['pct'] for d in data_list)
                    print(f"    {task:12s}: {last['pct']:3d}% "
                          f"(max: {max_pct}%)")

        print(f"{'='*60}\n")


# =========================================================================
# Fungsi pembacaan dari serial port
# =========================================================================
def read_serial(port, baud, parser):
    """Baca data dari serial port secara real-time."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial tidak terinstall!")
        print("  Install: pip install pyserial")
        sys.exit(1)

    print(f"[INFO] Membuka {port} @ {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"[INFO] Terhubung ke {port}")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti\n")

        while True:
            try:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='replace')
                    parser.parse_line(line)
            except KeyboardInterrupt:
                print("\n[INFO] Dihentikan oleh pengguna")
                break
            except Exception as e:
                print(f"[WARN] Error baca serial: {e}")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
        sys.exit(1)
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("[INFO] Serial port ditutup")


# =========================================================================
# Fungsi pembacaan dari file log
# =========================================================================
def read_file(filepath, parser):
    """Baca data dari file log yang sudah direkam."""
    if not os.path.exists(filepath):
        print(f"[ERROR] File tidak ditemukan: {filepath}")
        sys.exit(1)

    print(f"[INFO] Membaca file: {filepath}")
    line_count = 0

    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                parser.parse_line(line)
                line_count += 1
    except Exception as e:
        print(f"[ERROR] Gagal membaca file: {e}")
        sys.exit(1)

    print(f"[INFO] Selesai membaca {line_count} baris")


# =========================================================================
# Main entry point
# =========================================================================
def main():
    """Fungsi utama program debug parser."""
    arg_parser = argparse.ArgumentParser(
        description='Debug Serial Parser - STM32 Task Stack Monitor',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  %(prog)s --port /dev/ttyUSB0
  %(prog)s --port COM3 --baud 115200
  %(prog)s --file capture.log
  %(prog)s --file capture.log --plot
        """
    )

    arg_parser.add_argument('--port', '-p', type=str,
                            help='Serial port (mis: /dev/ttyUSB0, COM3)')
    arg_parser.add_argument('--baud', '-b', type=int, default=115200,
                            help='Baud rate (default: 115200)')
    arg_parser.add_argument('--file', '-f', type=str,
                            help='Baca dari file log')
    arg_parser.add_argument('--plot', action='store_true',
                            help='Tampilkan plot setelah selesai')
    arg_parser.add_argument('--save', '-s', type=str,
                            help='Simpan output ke file')

    args = arg_parser.parse_args()

    if not args.port and not args.file:
        arg_parser.print_help()
        print("\n[ERROR] Tentukan --port atau --file!")
        sys.exit(1)

    # Banner
    print("=" * 60)
    print(" STM32 FreeRTOS Stack Monitor - Debug Parser")
    print(" Program 06: Task Stack Usage Analysis")
    print("=" * 60)

    parser = StackMonitorParser()

    # Redirect output jika --save
    original_stdout = sys.stdout
    save_file = None
    if args.save:
        save_file = open(args.save, 'w', encoding='utf-8')
        sys.stdout = type('Tee', (), {
            'write': lambda self, s: (original_stdout.write(s),
                                       save_file.write(s)),
            'flush': lambda self: (original_stdout.flush(),
                                    save_file.flush())
        })()

    try:
        if args.file:
            read_file(args.file, parser)
        elif args.port:
            read_serial(args.port, args.baud, parser)

        # Ringkasan
        parser.print_summary()

        # Plot jika diminta
        if args.plot or args.file:
            parser.plot_results()

    finally:
        if save_file:
            save_file.close()
            sys.stdout = original_stdout
            print(f"[INFO] Output disimpan ke: {args.save}")


if __name__ == '__main__':
    main()

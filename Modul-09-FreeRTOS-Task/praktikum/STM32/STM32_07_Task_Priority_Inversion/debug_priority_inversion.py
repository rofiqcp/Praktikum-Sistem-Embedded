#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_07_Task_Priority_Inversion
==========================================================================
 Mem-parse output serial dari program Priority Inversion FreeRTOS.
 Mengekstrak data [DATA] tag, memvisualisasikan timeline priority
 inversion, dan menganalisis durasi blocking.

 Penggunaan:
   python debug_priority_inversion.py --port /dev/ttyUSB0
   python debug_priority_inversion.py --file capture.log
   python debug_priority_inversion.py --port COM3 --baud 115200

 Output:
   - Timeline diagram priority inversion
   - Gantt chart task execution
   - Statistik durasi blocking
   - Perbandingan antar skenario
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
    import matplotlib.patches as mpatches
    from matplotlib.collections import BrokenBarHCollection
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


# =========================================================================
# Kelas utama parser priority inversion
# =========================================================================
class PriorityInversionParser:
    """Parser dan visualizer untuk data priority inversion FreeRTOS."""

    def __init__(self):
        """Inisialisasi parser."""
        # Timeline events: [(time, task, event, priority)]
        self.timeline_events = []
        # Scenario data: {scenario_num: {key: value}}
        self.scenarios = defaultdict(dict)
        # Inversion measurements: [(scenario, blocked_ms, dwt_us)]
        self.inversions = []
        # Medium task work data: [(time, iter, high_blocked)]
        self.medium_work = []
        # Low task progress: [(time, pct)]
        self.low_progress = []
        # High task blocking: [(time, event)]
        self.high_events = []
        # Stats per scenario
        self.scenario_stats = []
        # Init config
        self.init_config = {}
        # Current scenario
        self.current_scenario = 0
        # Status
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial."""
        self.total_lines += 1
        line = line.strip()

        if not line:
            return

        # Cek tag [DATA]
        data_match = re.match(r'\[DATA\]\s+(\w+),(.*)', line)
        if not data_match:
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

        self._process_tag(tag, params, line)

    def _process_tag(self, tag, params, raw_line):
        """Proses data berdasarkan tag."""
        t = time.time() - self.start_time

        if tag == 'INIT':
            self.init_config = params
            print(f"\n{'='*60}")
            print(f" PRIORITY INVERSION DEMO - INIT")
            print(f"{'='*60}")
            print(f"  High Priority : {params.get('high_prio', '?')}")
            print(f"  Med Priority  : {params.get('med_prio', '?')}")
            print(f"  Low Priority  : {params.get('low_prio', '?')}")
            print(f"{'='*60}\n")

        elif tag == 'SCENARIO_START':
            # Parse scenario number
            num_match = re.search(r'SCENARIO_START,(\d+)', raw_line)
            if num_match:
                self.current_scenario = int(num_match.group(1))
            print(f"\n{'*'*60}")
            print(f" SKENARIO {self.current_scenario} DIMULAI")
            print(f"{'*'*60}\n")

        elif tag == 'EVENT':
            task = params.get('task', '?')
            event = params.get('event', '?')
            prio = params.get('prio', 0)
            tick = params.get('tick', 0)
            self.timeline_events.append({
                'time': t, 'task': task, 'event': event,
                'prio': prio, 'tick': tick,
                'scenario': self.current_scenario
            })
            # Emoji berdasarkan event
            emoji = '🔵'
            if 'BLOCKED' in event:
                emoji = '🔴'
            elif 'ACQUIRE' in event:
                emoji = '🔒'
            elif 'RELEASE' in event:
                emoji = '🔓'
            elif 'START' in event:
                emoji = '▶️'
            elif 'END' in event:
                emoji = '⏹️'
            elif 'UNBLOCKED' in event:
                emoji = '✅'
            print(f"  {emoji} [{tick:6d}] {task:12s} (P{prio}): {event}")

        elif tag == 'MEDIUM_WORK':
            iter_num = params.get('iter', 0)
            blocked = params.get('high_blocked', 0)
            tick = params.get('tick', 0)
            self.medium_work.append({
                'time': t, 'iter': iter_num,
                'high_blocked': blocked, 'tick': tick
            })
            status = "🔴 HIGH BLOCKED" if blocked else "✅ High free"
            print(f"    MediumTask iter {iter_num}: {status}")

        elif tag == 'LOW_PROGRESS':
            pct = params.get('pct', 0)
            tick = params.get('tick', 0)
            self.low_progress.append({
                'time': t, 'pct': pct, 'tick': tick
            })

        elif tag == 'HIGH_BLOCKED':
            owner = params.get('owner', 0)
            tick = params.get('tick', 0)
            self.high_events.append({
                'time': t, 'event': 'BLOCKED',
                'owner': owner, 'tick': tick
            })
            print(f"\n  🚫 HighTask TERBLOKIR! Resource dimiliki task {owner}")

        elif tag == 'HIGH_DONE':
            blocked_ms = params.get('blocked_ms', 0)
            tick = params.get('tick', 0)
            self.high_events.append({
                'time': t, 'event': 'DONE',
                'blocked_ms': blocked_ms, 'tick': tick
            })
            print(f"  ✅ HighTask selesai (blocked {blocked_ms} ms)")

        elif tag == 'INVERSION':
            blocked_ms = params.get('blocked_ms', 0)
            dwt_us = params.get('dwt_us', 0)
            scenario = params.get('scenario', self.current_scenario)
            self.inversions.append({
                'scenario': scenario,
                'blocked_ms': blocked_ms,
                'dwt_us': dwt_us, 'time': t
            })
            print(f"\n  📊 INVERSION: blocked={blocked_ms}ms, "
                  f"DWT={dwt_us}μs")

        elif tag == 'TIMELINE':
            idx = params.get('idx', 0)
            tick = params.get('tick', 0)
            task = params.get('task', '?')
            prio = params.get('prio', 0)
            event = params.get('event', '?')
            # Sudah di-print saat EVENT, skip duplicate

        elif tag == 'SCENARIO_END':
            inversion = params.get('inversion', 0)
            print(f"\n{'*'*60}")
            print(f" SKENARIO {self.current_scenario} SELESAI")
            print(f" Inversion terdeteksi: {'YA' if inversion else 'TIDAK'}")
            print(f"{'*'*60}\n")

        elif tag == 'STATS':
            scenario = params.get('scenario', 0)
            high_runs = params.get('high_runs', 0)
            med_runs = params.get('med_runs', 0)
            low_runs = params.get('low_runs', 0)
            self.scenario_stats.append({
                'scenario': scenario,
                'high': high_runs, 'med': med_runs, 'low': low_runs
            })

        elif tag == 'STACK_OVERFLOW':
            task = params.get('task', '?')
            print(f"\n  🔴 STACK OVERFLOW pada {task}!")

        elif tag == 'MALLOC_FAIL':
            print(f"\n  🔴 MALLOC FAILED!")

    def plot_results(self):
        """Buat visualisasi matplotlib."""
        if not HAS_MATPLOTLIB:
            print("\n[WARN] matplotlib tidak tersedia")
            return

        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('STM32 FreeRTOS Priority Inversion Analysis',
                     fontsize=14, fontweight='bold')

        # --- Plot 1: Timeline Gantt chart ---
        ax1 = axes[0, 0]
        task_colors = {
            'HighTask': '#e74c3c', 'MediumTask': '#f39c12',
            'LowTask': '#3498db', 'Orchestrator': '#95a5a6'
        }
        task_y = {'HighTask': 3, 'MediumTask': 2, 'LowTask': 1}

        for evt in self.timeline_events:
            task = evt['task']
            if task in task_y:
                color = task_colors.get(task, 'gray')
                marker = 'v' if 'BLOCKED' in evt['event'] else 'o'
                ax1.scatter(evt['time'], task_y[task],
                            c=color, s=80, marker=marker, zorder=5)

        ax1.set_yticks([1, 2, 3])
        ax1.set_yticklabels(['Low (P1)', 'Medium (P3)', 'High (P5)'])
        ax1.set_xlabel('Waktu (detik)')
        ax1.set_title('Timeline Task Events')
        ax1.grid(True, alpha=0.3)

        # Legend
        legend_elements = [
            plt.Line2D([0], [0], marker='o', color='w',
                       markerfacecolor=c, markersize=10, label=t)
            for t, c in task_colors.items() if t != 'Orchestrator'
        ]
        ax1.legend(handles=legend_elements, loc='upper right', fontsize=8)

        # --- Plot 2: Inversion durations ---
        ax2 = axes[0, 1]
        if self.inversions:
            scenarios = [inv['scenario'] for inv in self.inversions]
            blocked_ms = [inv['blocked_ms'] for inv in self.inversions]
            dwt_us = [inv['dwt_us'] for inv in self.inversions]

            x = range(len(scenarios))
            width = 0.35
            ax2.bar([i - width/2 for i in x], blocked_ms, width,
                    label='Blocked (ms)', color='#e74c3c')
            ax2.bar([i + width/2 for i in x],
                    [d / 1000 for d in dwt_us], width,
                    label='DWT (ms)', color='#3498db')

            ax2.set_xlabel('Skenario')
            ax2.set_ylabel('Durasi (ms)')
            ax2.set_title('Durasi Priority Inversion')
            ax2.set_xticks(list(x))
            ax2.set_xticklabels([f'S{s}' for s in scenarios])
            ax2.legend()
            ax2.grid(True, alpha=0.3, axis='y')
        else:
            ax2.text(0.5, 0.5, 'Menunggu data inversion...',
                     ha='center', va='center', transform=ax2.transAxes)

        # --- Plot 3: Medium task iterations vs High blocked ---
        ax3 = axes[1, 0]
        if self.medium_work:
            iters = [d['iter'] for d in self.medium_work]
            blocked = [d['high_blocked'] for d in self.medium_work]
            colors_mw = ['#e74c3c' if b else '#2ecc71' for b in blocked]
            ax3.bar(range(len(iters)), iters, color=colors_mw)
            ax3.set_xlabel('Sample')
            ax3.set_ylabel('Iterasi')
            ax3.set_title('MediumTask Work (merah=High blocked)')
            ax3.grid(True, alpha=0.3, axis='y')
        else:
            ax3.text(0.5, 0.5, 'Menunggu data MediumTask...',
                     ha='center', va='center', transform=ax3.transAxes)

        # --- Plot 4: Priority Inversion Diagram ---
        ax4 = axes[1, 1]
        ax4.set_xlim(0, 10)
        ax4.set_ylim(0, 6)
        ax4.set_title('Diagram Priority Inversion')

        # Draw simplified diagram
        # Low task holding resource
        ax4.barh(1, 4, left=0, height=0.6, color='#3498db',
                 alpha=0.7, label='LowTask (resource)')
        ax4.barh(1, 2, left=6, height=0.6, color='#3498db', alpha=0.3)

        # Medium task running during inversion
        ax4.barh(3, 4, left=3, height=0.6, color='#f39c12',
                 alpha=0.7, label='MediumTask (running)')

        # High task blocked then running
        ax4.barh(5, 4, left=2, height=0.6, color='#e74c3c',
                 alpha=0.3, label='HighTask (blocked)')
        ax4.barh(5, 2, left=7, height=0.6, color='#e74c3c', alpha=0.7)

        # Annotations
        ax4.annotate('INVERSION!', xy=(5, 4), fontsize=12,
                     fontweight='bold', color='red', ha='center')
        ax4.arrow(5, 3.8, 0, -0.5, head_width=0.2,
                  head_length=0.1, fc='red', ec='red')

        ax4.set_yticks([1, 3, 5])
        ax4.set_yticklabels(['Low (P1)', 'Med (P3)', 'High (P5)'])
        ax4.set_xlabel('Waktu →')
        ax4.legend(loc='upper left', fontsize=8)
        ax4.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('priority_inversion_analysis.png', dpi=150,
                    bbox_inches='tight')
        print(f"\n[INFO] Grafik disimpan: priority_inversion_analysis.png")
        plt.show()

    def print_summary(self):
        """Cetak ringkasan akhir."""
        elapsed = time.time() - self.start_time
        print(f"\n{'='*60}")
        print(f" RINGKASAN PARSING - PRIORITY INVERSION")
        print(f"{'='*60}")
        print(f"  Durasi          : {elapsed:.1f} detik")
        print(f"  Total baris     : {self.total_lines}")
        print(f"  Data baris      : {self.data_lines}")
        print(f"  Timeline events : {len(self.timeline_events)}")
        print(f"  Skenario        : {self.current_scenario}")
        print(f"  Inversions      : {len(self.inversions)}")

        if self.inversions:
            avg_blocked = sum(i['blocked_ms'] for i in self.inversions) \
                          / len(self.inversions)
            print(f"  Rata-rata blocked: {avg_blocked:.1f} ms")

        if self.scenario_stats:
            print(f"\n  Statistik per skenario:")
            for s in self.scenario_stats:
                print(f"    S{s['scenario']}: High={s['high']}, "
                      f"Med={s['med']}, Low={s['low']}")

        print(f"{'='*60}\n")


# =========================================================================
# Fungsi I/O
# =========================================================================
def read_serial(port, baud, parser):
    """Baca dari serial port."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial tidak terinstall! pip install pyserial")
        sys.exit(1)

    print(f"[INFO] Membuka {port} @ {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"[INFO] Terhubung. Tekan Ctrl+C untuk berhenti\n")
        while True:
            try:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='replace')
                    parser.parse_line(line)
            except KeyboardInterrupt:
                print("\n[INFO] Dihentikan")
                break
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
        sys.exit(1)
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def read_file(filepath, parser):
    """Baca dari file log."""
    if not os.path.exists(filepath):
        print(f"[ERROR] File tidak ditemukan: {filepath}")
        sys.exit(1)

    print(f"[INFO] Membaca: {filepath}")
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            parser.parse_line(line)
    print(f"[INFO] Selesai")


# =========================================================================
# Main
# =========================================================================
def main():
    """Entry point."""
    ap = argparse.ArgumentParser(
        description='Debug Parser - STM32 Priority Inversion Demo',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh:
  %(prog)s --port /dev/ttyUSB0
  %(prog)s --file capture.log --plot
        """
    )
    ap.add_argument('--port', '-p', type=str, help='Serial port')
    ap.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate')
    ap.add_argument('--file', '-f', type=str, help='File log input')
    ap.add_argument('--plot', action='store_true', help='Tampilkan plot')
    ap.add_argument('--save', '-s', type=str, help='Simpan output ke file')

    args = ap.parse_args()

    if not args.port and not args.file:
        ap.print_help()
        print("\n[ERROR] Tentukan --port atau --file!")
        sys.exit(1)

    print("=" * 60)
    print(" STM32 FreeRTOS Priority Inversion - Debug Parser")
    print(" Program 07: Priority Inversion Analysis")
    print("=" * 60)

    parser = PriorityInversionParser()

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

        parser.print_summary()
        if args.plot or args.file:
            parser.plot_results()
    finally:
        if save_file:
            save_file.close()
            sys.stdout = original_stdout


if __name__ == '__main__':
    main()

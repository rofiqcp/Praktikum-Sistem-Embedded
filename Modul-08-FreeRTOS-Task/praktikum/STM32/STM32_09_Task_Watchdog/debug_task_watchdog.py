#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_09_Task_Watchdog
==========================================================================
 Mem-parse output serial dari program Task Watchdog (IWDG) FreeRTOS.
 Mengekstrak data [DATA] tag, memvisualisasikan feed count, countdown
 ke reset, dan status task.

 Penggunaan:
   python debug_task_watchdog.py --port /dev/ttyUSB0
   python debug_task_watchdog.py --file capture.log
   python debug_task_watchdog.py --port COM3 --baud 115200

 Output:
   - Timeline feed IWDG
   - Countdown ke expected reset
   - Status task (normal, hang, feeder stop)
   - Deteksi alasan reset
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
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


# =========================================================================
# Kelas utama parser watchdog
# =========================================================================
class WatchdogParser:
    """Parser dan visualizer untuk data IWDG watchdog FreeRTOS."""

    def __init__(self):
        """Inisialisasi parser."""
        # Feed data: [(time, count, tick, active)]
        self.feed_data = []
        # Worker data: [(time, cycle, result)]
        self.worker_data = []
        # Problem task data: [(time, cycle, remaining, status)]
        self.problem_data = []
        # Monitor data: [(time, cycle, feeds, feeder, problem, worker, since_feed)]
        self.monitor_data = []
        # Countdown data: [(time, remaining_ms)]
        self.countdown_data = []
        # Reset events
        self.reset_events = []
        # Init config
        self.init_config = {}
        # IWDG config
        self.iwdg_config = {}
        # Status flags
        self.feeder_stopped = False
        self.problem_hung = False
        self.iwdg_reset_detected = False
        # Timing
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
            # Highlight important messages
            if '!!!!' in line:
                print(f"[{timestamp}] 🚨 {line}")
            elif 'PERINGATAN' in line or 'WARNING' in line:
                print(f"[{timestamp}] ⚠️  {line}")
            elif 'ERROR' in line:
                print(f"[{timestamp}] 🔴 {line}")
            else:
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
            print(f" TASK WATCHDOG (IWDG) - INIT")
            print(f"{'='*60}")
            print(f"  Feed Interval : {params.get('feed_interval', '?')} ms")
            print(f"  IWDG Timeout  : {params.get('timeout', '?')} ms")
            print(f"  Hang After    : {params.get('hang_after', '?')} siklus")
            print(f"  Stop After    : {params.get('stop_after', '?')} siklus")
            print(f"{'='*60}\n")

        elif tag == 'IWDG_CONFIG':
            self.iwdg_config = params
            print(f"  ⚙️  IWDG Config: prescaler={params.get('prescaler', '?')}, "
                  f"reload={params.get('reload', '?')}, "
                  f"timeout={params.get('timeout_ms', '?')}ms")

        elif tag == 'RESET_REASON':
            reset_type = params.get('type', 'UNKNOWN')
            self.reset_events.append({
                'time': t, 'type': reset_type
            })
            emoji = '🔄'
            if reset_type == 'IWDG_WATCHDOG':
                emoji = '🚨'
                self.iwdg_reset_detected = True
            elif reset_type == 'POWER_ON':
                emoji = '🔋'
            print(f"\n  {emoji} RESET REASON: {reset_type}")

        elif tag == 'POST_RESET_ANALYSIS':
            print(f"  📋 Post-reset analysis: {params.get('reason', '?')}")

        elif tag == 'FEED':
            count = params.get('count', 0)
            tick = params.get('tick', 0)
            active = params.get('active', 1)
            self.feed_data.append({
                'time': t, 'count': count,
                'tick': tick, 'active': active
            })
            # Visual heartbeat
            bar_len = min(count, 30)
            bar = '💚' * min(bar_len // 3, 10)
            print(f"  {bar} Feed #{count} (tick={tick})")

        elif tag == 'FEEDER_STOPPED':
            self.feeder_stopped = True
            feed_count = params.get('feed_count', 0)
            tick = params.get('tick', 0)
            print(f"\n  🛑 FEEDER BERHENTI setelah {feed_count} feeds!")
            print(f"     IWDG tidak akan di-refresh lagi!")

        elif tag == 'EXPECTED_RESET':
            tick = params.get('tick', 0)
            print(f"  ⏱️  Expected reset at tick: {tick}")

        elif tag == 'WORKER':
            cycle = params.get('cycle', 0)
            result = params.get('result', 0)
            tick = params.get('tick', 0)
            self.worker_data.append({
                'time': t, 'cycle': cycle,
                'result': result, 'tick': tick
            })

        elif tag == 'PROBLEM':
            cycle = params.get('cycle', 0)
            remaining = params.get('remaining', 0)
            status = params.get('status', 'normal')
            tick = params.get('tick', 0)
            self.problem_data.append({
                'time': t, 'cycle': cycle,
                'remaining': remaining, 'status': status, 'tick': tick
            })
            if remaining <= 3 and remaining > 0:
                print(f"  ⚠️  ProblemTask: {remaining} siklus lagi!")

        elif tag == 'PROBLEM_HANG':
            self.problem_hung = True
            cycle = params.get('cycle', 0)
            tick = params.get('tick', 0)
            print(f"\n  💀 PROBLEM TASK HANG! (siklus {cycle})")

        elif tag == 'COUNTDOWN':
            remaining_ms = params.get('remaining_ms', 0)
            tick = params.get('tick', 0)
            self.countdown_data.append({
                'time': t, 'remaining_ms': remaining_ms, 'tick': tick
            })
            secs = remaining_ms / 1000.0
            bar_filled = max(0, int((1 - secs / 4.0) * 20))
            bar_empty = 20 - bar_filled
            bar = '█' * bar_filled + '░' * bar_empty
            print(f"  ⏳ [{bar}] {secs:.1f}s sampai RESET")

        elif tag == 'RESET_OVERDUE':
            elapsed = params.get('elapsed', 0)
            print(f"  ⏰ Reset overdue! Elapsed: {elapsed}ms")

        elif tag == 'MONITOR':
            cycle = params.get('cycle', 0)
            feeds = params.get('feeds', 0)
            feeder = params.get('feeder', 1)
            problem = params.get('problem', 0)
            worker = params.get('worker', 0)
            since_feed = params.get('since_feed', 0)
            tick = params.get('tick', 0)
            self.monitor_data.append({
                'time': t, 'cycle': cycle, 'feeds': feeds,
                'feeder': feeder, 'problem': problem,
                'worker': worker, 'since_feed': since_feed,
                'tick': tick
            })
            status = "✅" if feeder and not problem else \
                     "⚠️" if feeder else "🔴"
            print(f"\n  {status} Monitor #{cycle}: feeds={feeds}, "
                  f"feeder={'ON' if feeder else 'OFF'}, "
                  f"problem={'HANG' if problem else 'OK'}, "
                  f"since_feed={since_feed}ms")

        elif tag == 'GUIDE_PRINTED':
            print(f"  📖 Panduan arsitektur watchdog ditampilkan")

        elif tag == 'STACK_OVERFLOW':
            task = params.get('task', '?')
            print(f"\n  🔴 STACK OVERFLOW: {task}")

        elif tag == 'MALLOC_FAIL':
            print(f"\n  🔴 MALLOC FAILED!")

    def plot_results(self):
        """Buat visualisasi matplotlib."""
        if not HAS_MATPLOTLIB:
            print("\n[WARN] matplotlib tidak tersedia")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('STM32 FreeRTOS IWDG Watchdog Analysis',
                     fontsize=14, fontweight='bold')

        # --- Plot 1: Feed count over time ---
        ax1 = axes[0, 0]
        if self.feed_data:
            times = [d['time'] for d in self.feed_data]
            counts = [d['count'] for d in self.feed_data]
            ax1.plot(times, counts, 'g-o', markersize=4,
                     label='Feed Count')
            ax1.fill_between(times, counts, alpha=0.2, color='green')

            # Mark feeder stop point
            if self.feeder_stopped and len(times) > 0:
                ax1.axvline(x=times[-1], color='red',
                            linestyle='--', label='Feeder Stopped')

            ax1.set_xlabel('Waktu (detik)')
            ax1.set_ylabel('Feed Count')
            ax1.set_title('IWDG Feed Count vs Waktu')
            ax1.legend()
            ax1.grid(True, alpha=0.3)
        else:
            ax1.text(0.5, 0.5, 'Menunggu data feed...',
                     ha='center', va='center', transform=ax1.transAxes)

        # --- Plot 2: Countdown to reset ---
        ax2 = axes[0, 1]
        if self.countdown_data:
            times = [d['time'] for d in self.countdown_data]
            remaining = [d['remaining_ms'] / 1000.0 for d in self.countdown_data]
            ax2.plot(times, remaining, 'r-s', markersize=5,
                     label='Sisa Waktu')
            ax2.fill_between(times, remaining, alpha=0.2, color='red')
            ax2.axhline(y=0, color='black', linestyle='-', linewidth=2)
            ax2.set_xlabel('Waktu (detik)')
            ax2.set_ylabel('Sisa Waktu (detik)')
            ax2.set_title('Countdown ke IWDG Reset')
            ax2.legend()
            ax2.grid(True, alpha=0.3)
        else:
            ax2.text(0.5, 0.5, 'Menunggu data countdown...',
                     ha='center', va='center', transform=ax2.transAxes)

        # --- Plot 3: Monitor status timeline ---
        ax3 = axes[1, 0]
        if self.monitor_data:
            times = [d['time'] for d in self.monitor_data]
            since_feed = [d['since_feed'] for d in self.monitor_data]
            feeder_status = [d['feeder'] for d in self.monitor_data]
            problem_status = [d['problem'] for d in self.monitor_data]

            # Since last feed
            colors_sf = ['green' if f else 'red' for f in feeder_status]
            ax3.bar(range(len(times)), since_feed, color=colors_sf,
                    alpha=0.7, label='Sejak Feed Terakhir (ms)')

            # IWDG timeout line
            timeout_ms = self.init_config.get('timeout', 4000)
            ax3.axhline(y=timeout_ms, color='red', linestyle='--',
                        linewidth=2, label=f'IWDG Timeout ({timeout_ms}ms)')

            ax3.set_xlabel('Monitor Cycle')
            ax3.set_ylabel('Waktu (ms)')
            ax3.set_title('Waktu Sejak Feed Terakhir')
            ax3.legend(fontsize=8)
            ax3.grid(True, alpha=0.3, axis='y')
        else:
            ax3.text(0.5, 0.5, 'Menunggu data monitor...',
                     ha='center', va='center', transform=ax3.transAxes)

        # --- Plot 4: System health status ---
        ax4 = axes[1, 1]
        if self.monitor_data:
            cycles = [d['cycle'] for d in self.monitor_data]
            # Health score: 100 if feeder on and no problem, else decrease
            health = []
            for d in self.monitor_data:
                score = 100
                if not d['feeder']:
                    score -= 50
                if d['problem']:
                    score -= 30
                if d['since_feed'] > 2000:
                    score -= 20
                health.append(max(0, score))

            colors_h = ['green' if h > 70 else 'orange' if h > 30 else 'red'
                        for h in health]
            ax4.bar(range(len(cycles)), health, color=colors_h, alpha=0.8)
            ax4.set_xlabel('Monitor Cycle')
            ax4.set_ylabel('Health Score (%)')
            ax4.set_title('System Health Score')
            ax4.set_ylim(0, 110)

            # Annotate events
            for i, h in enumerate(health):
                if h < 30:
                    ax4.annotate('KRITIS!', (i, h + 5), ha='center',
                                fontsize=7, color='red', fontweight='bold')

            ax4.grid(True, alpha=0.3, axis='y')
        else:
            ax4.text(0.5, 0.5, 'Menunggu data...',
                     ha='center', va='center', transform=ax4.transAxes)

        plt.tight_layout()
        plt.savefig('task_watchdog_analysis.png', dpi=150,
                    bbox_inches='tight')
        print(f"\n[INFO] Grafik disimpan: task_watchdog_analysis.png")
        plt.show()

    def print_summary(self):
        """Cetak ringkasan akhir."""
        elapsed = time.time() - self.start_time
        print(f"\n{'='*60}")
        print(f" RINGKASAN PARSING - TASK WATCHDOG")
        print(f"{'='*60}")
        print(f"  Durasi          : {elapsed:.1f} detik")
        print(f"  Total baris     : {self.total_lines}")
        print(f"  Data baris      : {self.data_lines}")
        print(f"  Feed events     : {len(self.feed_data)}")
        print(f"  Monitor cycles  : {len(self.monitor_data)}")
        print(f"  Reset events    : {len(self.reset_events)}")
        print(f"  Feeder stopped  : {'Ya' if self.feeder_stopped else 'Tidak'}")
        print(f"  Problem hung    : {'Ya' if self.problem_hung else 'Tidak'}")
        print(f"  IWDG reset      : {'Ya' if self.iwdg_reset_detected else 'Tidak'}")

        if self.feed_data:
            total_feeds = self.feed_data[-1]['count'] if self.feed_data else 0
            print(f"  Total feeds     : {total_feeds}")

        if self.countdown_data:
            min_remaining = min(d['remaining_ms'] for d in self.countdown_data)
            print(f"  Min countdown   : {min_remaining}ms")

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
        description='Debug Parser - STM32 Task Watchdog (IWDG)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh:
  %(prog)s --port /dev/ttyUSB0
  %(prog)s --file capture.log --plot
  %(prog)s --port COM3 -b 115200 --save output.txt
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
    print(" STM32 FreeRTOS Task Watchdog - Debug Parser")
    print(" Program 09: IWDG Watchdog Analysis")
    print("=" * 60)

    parser = WatchdogParser()

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
            print(f"[INFO] Output disimpan: {args.save}")


if __name__ == '__main__':
    main()

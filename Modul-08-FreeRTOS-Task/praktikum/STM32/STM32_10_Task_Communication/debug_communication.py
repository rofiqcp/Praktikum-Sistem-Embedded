#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_10_Task_Communication (Race Condition)
==========================================================================
 Mem-parse output serial dari program Race Condition demo.
 Mengekstrak data [DATA]RACE dan statistik korupsi data akibat
 akses shared variable tanpa proteksi mutex/queue.

 Penggunaan:
   python debug_communication.py --port /dev/ttyUSB0
   python debug_communication.py --file capture.log
   python debug_communication.py --demo

 Output:
   - Grafik corruption rate over time
   - Cumulative corruption count
   - Race condition frequency analysis
   - Detail jenis korupsi (checksum/sequence/pattern)
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


class RaceConditionParser:
    """Parser dan visualizer untuk data race condition."""

    def __init__(self):
        self.race_data = []        # [(time, cnt, corrupt, total, rate)]
        self.monitor_data = []     # [(time, cycle, writes, reads, corrupt, good, overall, recent, heap)]
        self.corrupt_detail = []   # [(time, type, chk, seq, pat)]
        self.consumer_events = []  # [(time, result, counter)]  result: 'ok'|'corrupt'
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()

    def parse_line(self, line):
        self.total_lines += 1
        line = line.strip()
        if not line:
            return
        t = time.time() - self.start_time
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

        # Parse [DATA]RACE cnt=X,corrupt=X,total=X,rate=X
        m = re.search(
            r'\[DATA\]RACE\s+cnt=(\d+),corrupt=(\d+),total=(\d+),rate=(\d+)',
            line
        )
        if m:
            cnt = int(m.group(1))
            corrupt = int(m.group(2))
            total = int(m.group(3))
            rate = int(m.group(4))
            self.race_data.append((t, cnt, corrupt, total, rate))
            self.data_lines += 1
            if rate > 0:
                print(f"[{ts}] 🔴 RACE cnt={cnt} corrupt={corrupt}/{total} rate={rate}%")
            else:
                print(f"[{ts}] 📊 RACE cnt={cnt} corrupt={corrupt}/{total} rate={rate}%")
            return

        # Parse [DATA]MONITOR,cycle=...,writes=...,reads=...,...
        m2 = re.search(
            r'\[DATA\]MONITOR,cycle=(\d+),writes=(\d+),reads=(\d+),'
            r'corrupt=(\d+),good=(\d+),overall=(\d+),recent=(\d+),'
            r'chk=(\d+),seq=(\d+),pat=(\d+),heap=(\d+)',
            line
        )
        if m2:
            self.monitor_data.append((
                t,
                int(m2.group(1)),   # cycle
                int(m2.group(2)),   # writes
                int(m2.group(3)),   # reads
                int(m2.group(4)),   # corrupt
                int(m2.group(5)),   # good
                int(m2.group(6)),   # overall rate
                int(m2.group(7)),   # recent rate
                int(m2.group(11)),  # heap
            ))
            self.data_lines += 1
            return

        # Parse [DATA]CORRUPT_DETAIL
        m3 = re.search(
            r'\[DATA\]CORRUPT_DETAIL,type=(\w+),chk_fail=(\d+),seq_fail=(\d+),pat_fail=(\d+)',
            line
        )
        if m3:
            self.corrupt_detail.append((
                t, m3.group(1), int(m3.group(2)), int(m3.group(3)), int(m3.group(4))
            ))
            self.data_lines += 1
            print(f"[{ts}] 🔍 Corrupt type={m3.group(1)} chk={m3.group(2)} seq={m3.group(3)} pat={m3.group(4)}")
            return

        # Parse consumer OK/CORRUPT lines
        if '[CONS]' in line and 'CORRUPT' in line:
            m4 = re.search(r'cnt=(\d+)', line)
            cnt = int(m4.group(1)) if m4 else 0
            self.consumer_events.append((t, 'corrupt', cnt))
            print(f"[{ts}] ❌ {line[:80]}")
            return

        if '[CONS]' in line and 'OK' in line:
            m5 = re.search(r'cnt=(\d+)', line)
            cnt = int(m5.group(1)) if m5 else 0
            self.consumer_events.append((t, 'ok', cnt))
            return

        # Parse producer writes (only print occasionally)
        if '[PROD]' in line and 'Write' in line:
            self.data_lines += 1
            return

        # Generic interesting lines
        if 'INIT' in line or 'EXPLAIN' in line:
            print(f"[{ts}] ℹ️  {line}")
        elif 'KORUPSI' in line or 'CORRUPT' in line.upper():
            print(f"[{ts}] 🚨 {line}")

    def print_summary(self):
        print(f"\n{'='*60}")
        print(f" RINGKASAN RACE CONDITION")
        print(f"{'='*60}")
        print(f"  Total lines parsed   : {self.total_lines}")
        print(f"  Data lines           : {self.data_lines}")
        print(f"  Race data points     : {len(self.race_data)}")
        print(f"  Monitor reports      : {len(self.monitor_data)}")
        print(f"  Consumer events      : {len(self.consumer_events)}")
        ok_count = sum(1 for _, r, _ in self.consumer_events if r == 'ok')
        corrupt_count = sum(1 for _, r, _ in self.consumer_events if r == 'corrupt')
        print(f"  Consumer OK          : {ok_count}")
        print(f"  Consumer CORRUPT     : {corrupt_count}")
        if self.race_data:
            last = self.race_data[-1]
            print(f"  Last counter         : {last[1]}")
            print(f"  Total corruptions    : {last[2]}")
            print(f"  Total reads          : {last[3]}")
            print(f"  Final rate           : {last[4]}%")
        if self.corrupt_detail:
            last_d = self.corrupt_detail[-1]
            print(f"  Checksum failures    : {last_d[2]}")
            print(f"  Sequence failures    : {last_d[3]}")
            print(f"  Pattern failures     : {last_d[4]}")
        print(f"{'='*60}")

    def plot(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] matplotlib tidak tersedia, skip plot.")
            return
        if not self.race_data:
            print("[WARN] Tidak ada data untuk di-plot.")
            return

        fig, axes = plt.subplots(3, 1, figsize=(13, 10), sharex=True)
        fig.suptitle('STM32_10 Race Condition Analysis (Unsafe Communication)', fontsize=14)

        times = [r[0] for r in self.race_data]
        cnts = [r[1] for r in self.race_data]
        corrupts = [r[2] for r in self.race_data]
        totals = [r[3] for r in self.race_data]
        rates = [r[4] for r in self.race_data]

        # Plot 1: Corruption rate over time
        ax1 = axes[0]
        ax1.plot(times, rates, 'r-o', markersize=3, linewidth=2, label='Corruption Rate (%)')
        ax1.fill_between(times, rates, alpha=0.2, color='red')
        ax1.set_ylabel('Rate (%)')
        ax1.set_title('Data Corruption Rate Over Time')
        ax1.grid(True, alpha=0.3)
        ax1.legend(loc='upper right', fontsize=8)

        # Plot 2: Cumulative corruptions vs total reads
        ax2 = axes[1]
        ax2.plot(times, totals, 'b-', linewidth=2, label='Total Reads')
        ax2.plot(times, corrupts, 'r-', linewidth=2, label='Corruptions')
        ax2.fill_between(times, corrupts, alpha=0.3, color='red')
        goods = [t - c for t, c in zip(totals, corrupts)]
        ax2.fill_between(times, goods, totals, alpha=0.15, color='blue')
        ax2.set_ylabel('Count')
        ax2.set_title('Cumulative Reads: OK vs Corrupt')
        ax2.grid(True, alpha=0.3)
        ax2.legend(loc='upper left', fontsize=8)

        # Plot 3: Corruption frequency (per-interval)
        ax3 = axes[2]
        if len(corrupts) > 1:
            deltas = [corrupts[i] - corrupts[i - 1] for i in range(1, len(corrupts))]
            delta_times = times[1:]
            colors = ['red' if d > 0 else '#4CAF50' for d in deltas]
            ax3.bar(delta_times, deltas, width=0.5, color=colors, alpha=0.7)
        ax3.set_xlabel('Time (s)')
        ax3.set_ylabel('New Corruptions')
        ax3.set_title('Race Condition Frequency (new corruptions per interval)')
        ax3.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('race_condition_analysis.png', dpi=150)
        print("[INFO] Plot disimpan: race_condition_analysis.png")
        plt.show()


def generate_demo_data():
    """Generate simulated race condition data."""
    import random
    random.seed(42)
    lines = []
    lines.append("=== Race Condition Demo (UNSAFE Communication) ===")
    lines.append("[DATA]INIT,prod_ms=20,cons_ms=23,data_size=8,work_us=50")
    lines.append("[DATA]EXPLAIN,printed=1")
    lines.append("[PROD] Producer task dimulai")
    lines.append("[CONS] Consumer task dimulai")

    corrupt_total = 0
    total_reads = 0
    chk_fail = 0
    seq_fail = 0
    pat_fail = 0

    for i in range(1, 201):
        total_reads += 1
        is_corrupt = random.random() < 0.12  # ~12% corruption rate

        if is_corrupt:
            corrupt_total += 1
            fail_type = random.choice(['checksum', 'sequence', 'pattern'])
            if fail_type == 'checksum':
                chk_fail += 1
            elif fail_type == 'sequence':
                seq_fail += 1
            else:
                pat_fail += 1

            rate = (corrupt_total * 100) // total_reads
            lines.append(
                f"[CONS] Read #{total_reads}: cnt={i} ts={i*20} seq={i} → CORRUPT! ❌"
            )
            lines.append(f"       Reason: {fail_type.upper()} mismatch")
            lines.append(
                f"[DATA]RACE cnt={i},corrupt={corrupt_total},total={total_reads},rate={rate}"
            )
            lines.append(
                f"[DATA]CORRUPT_DETAIL,type={fail_type},"
                f"chk_fail={chk_fail},seq_fail={seq_fail},pat_fail={pat_fail}"
            )
        else:
            if total_reads % 100 == 0:
                lines.append(f"[CONS] Read #{total_reads}: cnt={i} ts={i*20} → OK ✅")

        # Producer write messages every 50
        if i % 50 == 0:
            lines.append(f"[PROD] Write #{i}: cnt={i} ts={i*20} seq={i} chk=0x{random.randint(0,0xFFFF):08X}")

        # Monitor every 50 reads
        if total_reads % 50 == 0:
            rate = (corrupt_total * 100) // total_reads if total_reads > 0 else 0
            lines.append(
                f"[DATA]RACE cnt={i},corrupt={corrupt_total},total={total_reads},rate={rate}"
            )
            lines.append(
                f"[DATA]MONITOR,cycle={total_reads//50},writes={i},reads={total_reads},"
                f"corrupt={corrupt_total},good={total_reads - corrupt_total},"
                f"overall={rate},recent={random.randint(0,20)},"
                f"chk={chk_fail},seq={seq_fail},pat={pat_fail},heap=4200"
            )

    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug parser untuk STM32_10 Race Condition Demo')
    parser.add_argument('--port', type=str, help='Serial port (e.g. /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', type=str, help='File log untuk di-parse')
    parser.add_argument('--demo', action='store_true', help='Jalankan dengan data demo')
    args = parser.parse_args()

    p = RaceConditionParser()

    if args.demo:
        print("[INFO] Mode DEMO - menggunakan data simulasi\n")
        for line in generate_demo_data():
            p.parse_line(line)
            time.sleep(0.005)
        p.print_summary()
        p.plot()
    elif args.file:
        print(f"[INFO] Membaca file: {args.file}\n")
        with open(args.file, 'r', errors='replace') as f:
            for line in f:
                p.parse_line(line)
        p.print_summary()
        p.plot()
    elif args.port:
        if not HAS_SERIAL:
            print("[ERROR] pyserial belum terinstall. pip install pyserial")
            sys.exit(1)
        print(f"[INFO] Membuka {args.port} @ {args.baud} baud\n")
        try:
            ser = serial.Serial(args.port, args.baud, timeout=1)
            while True:
                raw = ser.readline()
                if raw:
                    p.parse_line(raw.decode('utf-8', errors='replace'))
        except KeyboardInterrupt:
            print("\n[INFO] Dihentikan oleh user.")
            ser.close()
            p.print_summary()
            p.plot()
        except serial.SerialException as e:
            print(f"[ERROR] Serial: {e}")
            sys.exit(1)
    else:
        parser.print_help()
        print("\nContoh: python debug_communication.py --demo")


if __name__ == '__main__':
    main()
